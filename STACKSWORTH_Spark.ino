/***************************************************************************************
 *  STACKSWORTH Spark – "mainScreen" UI
 *  --------------------------------------------------
 *  Project     : STACKSWORTH Spark Firmware
 *  Version     : v1.0.0
 *  Device      : ESP32-S3 Waveshare 7" Touchscreen (800x480)
 *  Description : Modular Bitcoin Dashboard UI using LVGL
 *  Designer    : Bitcoin Manor 🟧
 * 
 *  
 *  💡 Easter Egg: Try tapping the infinity label in v0.1 😉
 ***************************************************************************************/




#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Waveshare_ST7262_LVGL.h>
#include <lvgl.h>
#include <time.h>
#include "metrics_screen.h"
#include "bigstats_screen.h"
#include "world_screen.h"
#include "bitaxe_screen.h"
#include "settings_screen.h"
#include "screen_manager.h"
#include "ui_theme.h"
#include <float.h>
#include "esp_system.h"
#include "esp_wifi.h"   // for esp_read_mac
#include "data_store.h"
#include <math.h>
#include "axestack_theme.h"

// ===== Forward Function Declarations =====
String formatUptime(unsigned long milliseconds);
bool connectWiFiFromPrefs(uint32_t timeoutMs = 15000);
void startPortal();
void setupWebRoutes();
void showportal_screen(const String& apName);
void start_portal_save_animation();

// ===== SatoNak API Configuration =====
#define USE_SATONAK_API 1    // 1 = use SatoNak endpoints, 0 = fallback to original sources

static const char* SATONAK_BASE   = "https://satonak.bitcoinmanor.com";
static const char* SATONAK_PRICE  = "/api/price";   // supports ?fiat=EUR etc.
static const char* SATONAK_HEIGHT = "/api/height";  
static const char* SATONAK_MINER  = "/api/miner";   
static const char* SATONAK_FEE    = "/api/fee";     
static const char* SATONAK_HASHRATE = "/api/hashrate";
static const char* SATONAK_CIRCSUPPLY = "/api/circsupply";
static const char* SATONAK_ATH = "/api/ath";
static const char* SATONAK_CHANGE24H = "/api/change24h";
static const char* SATONAK_DAYS_SINCE_ATH = "/api/days_since_ath";

// ===== World/Weather globals =====
static String g_savedCity;
static String g_savedTZ;     // e.g. "America/Edmonton"
static String g_savedTopText;   // 📝 Custom top row message (max 10 chars)
static String g_savedBottomText;// 📝 Custom bottom row message (max 10 chars)
static String g_savedCurrency;  // 🌍 User's preferred currency (USD, EUR, etc.)
static String g_savedTheme;     // 🎨 User's preferred theme (scroll, fade)
static float  g_lat = 0.0f;
static float  g_lon = 0.0f;

// ===== Cached display values (persisted between screens) =====
String lastPrice      = "";
String lastFee        = "";
String lastBlockHeight = "";
String lastMiner      = "";


// Default fiat (will be loaded from preferences)
static String getCurrentFiatCode() {
  return g_savedCurrency.length() > 0 ? g_savedCurrency : "USD";
}

// Get currency symbol for display
static String getCurrencySymbol() {
  String fiat = getCurrentFiatCode();
  if (fiat == "USD") return "$";
  if (fiat == "CAD") return "$";       // Clean $ since top row shows "CAD PRICE"
  if (fiat == "EUR") return "";        // Clean number since top row shows "EUR PRICE"  
  if (fiat == "GBP") return "";        // Clean number since top row shows "GBP PRICE"
  if (fiat == "JPY") return "";        // Clean number since top row shows "JPY PRICE" 
  if (fiat == "AUD") return "$";       // Clean $ since top row shows "AUD PRICE"
  if (fiat == "CHF") return "";        // Clean number since top row shows "CHF PRICE"
  if (fiat == "CNY") return "";        // Clean number since top row shows "CNY PRICE"
  if (fiat == "SEK") return "";        // Clean number since top row shows "SEK PRICE"
  if (fiat == "NOK") return "";        // Clean number since top row shows "NOK PRICE"
  return ""; // fallback to no symbol
}

static inline String satonakUrl(const char* path, const char* fiat = nullptr) {
  String u = String(SATONAK_BASE) + String(path);
  if (fiat && fiat[0] != '\0') {
    u += "?fiat="; u += fiat;
  }
  return u;
}

void ui_update_blocks_to_million(long height);

void ui_update_fee_badges_lmh(int low, int med, int high);

// Forward decls from metrics_screen.cpp so the .ino can call them
void ui_update_mid_metrics(float hashrateEh,
                           float diffPct,
                           int   diffDaysAgo,
                           float marketCapUsd,
                           float circBtc,
                           float athUsd,
                           int   athDaysAgo,
                           float fromAthPct);

void ui_update_block_intervals(const uint8_t* minutes, int count);
void ui_update_block_labels(const uint32_t* heights, int count);





// ==== Captive portal globals & helpers ====
AsyncWebServer server(80);
AsyncWebSocket ws("/ws/logs");  // WebSocket for live logs
DNSServer dns;
Preferences prefs;

IPAddress apIP(192,168,4,1);
bool portalModeActive = false;
bool lv_ready = false;        // set true after lcd_init/LVGL init
String g_apName;              // <-- needed by startPortal + LVGL pre-screen

// mDNS and WiFi monitoring globals
bool mdnsActive = false;
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 30000;  // Check WiFi every 30 seconds

// Live logging globals
String logBuffer = "";
const size_t LOG_BUFFER_SIZE = 2048;
bool logClientsConnected = false;

// WebSocket event handler
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch(type) {
    case WS_EVT_CONNECT:
      Serial.printf("📡 WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      logClientsConnected = true;
      // Send recent log buffer to new client
      if (logBuffer.length() > 0) {
        client->text(logBuffer);
      }
      break;
      
    case WS_EVT_DISCONNECT:
      Serial.printf("📴 WebSocket client #%u disconnected\n", client->id());
      if (ws.count() == 0) {
        logClientsConnected = false;
      }
      break;
      
    case WS_EVT_ERROR:
      Serial.printf("❌ WebSocket error from client #%u: %s\n", client->id(), (char*)data);
      break;
      
    case WS_EVT_PONG:
    case WS_EVT_DATA:
      // Handle incoming data if needed
      break;
  }
}

// Enhanced Serial.print functions that also send to WebSocket
void logPrint(String message) {
  Serial.print(message);
  
  if (logClientsConnected && ws.count() > 0) {
    // Add to buffer
    logBuffer += message;
    
    // Trim buffer if too large
    if (logBuffer.length() > LOG_BUFFER_SIZE) {
      int cutPos = logBuffer.indexOf('\n', logBuffer.length() - LOG_BUFFER_SIZE + 200);
      if (cutPos > 0) {
        logBuffer = logBuffer.substring(cutPos + 1);
      } else {
        logBuffer = logBuffer.substring(logBuffer.length() - LOG_BUFFER_SIZE + 200);
      }
    }
    
    // Send to all connected clients
    ws.textAll(message);
  }
}

void logPrintln(String message) {
  logPrint(message + "\n");
}

static const char* AP_PREFIX = "STACKSWORTH-SPARK";  // Spark build; Matrix uses ...-MATRIX

static const char* PORTAL_FILE = "/STACKS_Wifi_Portal.html";


String makeAPName() {
  uint8_t mac[6] = {0};

  // We already call WiFi.mode(WIFI_AP) before this, so AP MAC is available.
  esp_err_t err = esp_wifi_get_mac(WIFI_IF_AP, mac);
  if (err != ESP_OK) {
    // Fallback: try station MAC if AP MAC wasn't ready yet
    esp_wifi_get_mac(WIFI_IF_STA, mac);
  }

  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(AP_PREFIX) + "-" + suffix;  // e.g., STACKSWORTH-SPARK-9C1A2B
}


// Initialize mDNS for spark.local access
void setupMDNS() {
  if (!mdnsActive && WiFi.status() == WL_CONNECTED) {
    if (MDNS.begin("spark")) {
      logPrintln("✅ mDNS started: http://spark.local");
      
      // Add service advertisements
      MDNS.addService("http", "tcp", 80);
      MDNS.addServiceTxt("http", "tcp", "device", "STACKSWORTH-Spark");
      MDNS.addServiceTxt("http", "tcp", "version", "v1.1.1");
      
      mdnsActive = true;
    } else {
      logPrintln("❌ mDNS failed to start");
    }
  }
}

// Setup web server for dashboard mode (when connected to WiFi)
void setupDashboardServer() {
  if (!SPIFFS.begin(true)) {
    Serial.println("⚠️ SPIFFS mount failed");
    return;
  }
  
  // Debug: List all files in SPIFFS
  logPrintln("📂 SPIFFS Files:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file) {
    logPrintln("  📄 " + String(file.name()) + " (" + String(file.size()) + " bytes)");
    file = root.openNextFile();
  }
  
  setupWebRoutes();  // Setup all routes
  server.begin();
  logPrintln("🌐 Dashboard server started");
}

// Generate fallback dashboard HTML if file missing
String generateDashboardFallback() {
  String html = R"html(<!DOCTYPE html>
<html><head><title>STACKSWORTH Spark Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #000, #1a1a1a); color: #fff; padding: 20px; min-height: 100vh; }
.container { max-width: 800px; margin: 0 auto; }
.header { text-align: center; margin-bottom: 30px; border-bottom: 2px solid #fca420; padding-bottom: 20px; }
.header h1 { color: #fca420; font-size: 2.5rem; margin-bottom: 10px; text-shadow: 0 0 10px rgba(252,164,32,0.5); }
.card { background: rgba(26,26,26,0.8); border-radius: 15px; padding: 25px; margin: 20px 0; border: 2px solid #333; }
.card:hover { border-color: #fca420; }
.info-row { display: flex; justify-content: space-between; margin: 10px 0; }
.btn { background: linear-gradient(135deg, #fca420, #ff8800); color: #000; border: none; padding: 12px 20px; border-radius: 8px; cursor: pointer; font-weight: bold; margin: 5px; }
.btn:hover { background: linear-gradient(135deg, #ff8800, #fca420); }
.status-online { color: #00ff88; }
</style>
</head><body>
<div class="container">
<div class="header">
<h1>STACKSWORTH SPARK</h1>
<div style="color: #ccc;">Dashboard Fallback Mode • )html";

  html += WiFi.localIP().toString();
  html += R"html(</div>
</div>
<div class="card">
<h3 style="color: #fca420; margin-bottom: 15px;">📄 Dashboard File Missing</h3>
<p style="margin-bottom: 15px;">The main dashboard.html file was not found in SPIFFS.</p>
<p style="margin-bottom: 15px;"><strong>To fix this:</strong></p>
<ol style="margin-left: 20px; line-height: 1.6;">
<li>Use Arduino IDE → Tools → ESP32 Sketch Data Upload</li>
<li>Upload the /data folder contents to SPIFFS</li>
<li>Refresh this page to see the full dashboard</li>
</ol>
</div>
<div class="card">
<h3 style="color: #fca420; margin-bottom: 15px;">🔧 Quick Actions</h3>
<div class="info-row"><span>Device:</span><span>STACKSWORTH-SPARK</span></div>
<div class="info-row"><span>IP Address:</span><span class="status-online">)html";

  html += WiFi.localIP().toString();
  html += R"html(</span></div>
<div class="info-row"><span>mDNS:</span><span class="status-online">http://spark.local</span></div>
<div class="info-row"><span>WiFi:</span><span class="status-online">)html";

  html += WiFi.SSID();
  html += R"html(</span></div>
<div style="margin-top: 20px;">
<button class="btn" onclick="fetch('/api/status').then(r=>r.json()).then(d=>alert(JSON.stringify(d,null,2)))">📊 Device Status</button>
<button class="btn" onclick="if(confirm('Reboot device?')){fetch('/api/reboot',{method:'POST'}).then(()=>alert('Rebooting...'))}">🔄 Reboot</button>
<button class="btn" onclick="window.location.reload()">🔄 Refresh</button>
</div>
</div>
<div style="text-align: center; margin-top: 30px; color: #777;">
<p>STACKSWORTH Spark v1.1.1 • <a href="http://spark.local" style="color: #fca420;">http://spark.local</a></p>
</div>
</div>
</body></html>)html";

  return html;
}

// API Handler: Device Status
void handleAPIStatus(AsyncWebServerRequest* request) {
  StaticJsonDocument<1024> doc;
  
  // Device info
  doc["deviceName"] = makeAPName();
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["uptime"] = formatUptime(millis());
  doc["freeMemory"] = ESP.getFreeHeap() / 1024;  // KB
  
  // Network info
  doc["localIP"] = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  doc["rssi"] = WiFi.RSSI();
  doc["macAddress"] = WiFi.macAddress();
  
  // Load current settings
  prefs.begin("sw", true);
  doc["city"] = prefs.getString("city", "");
  doc["timezone"] = prefs.getString("tz", "11");
  doc["currency"] = prefs.getString("currency", "USD");
  doc["toptext"] = prefs.getString("toptext", "");
  doc["bottomtext"] = prefs.getString("bottomtext", "");
  prefs.end();
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

// API Handler: Save Settings
void handleAPISettings(AsyncWebServerRequest* request, uint8_t *data, size_t len) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, data, len);
  
  StaticJsonDocument<128> response;
  
  if (error) {
    response["success"] = false;
    response["error"] = "Invalid JSON";
  } else {
    prefs.begin("sw", false);
    
    if (doc.containsKey("city")) {
      prefs.putString("city", doc["city"].as<String>());
      g_savedCity = doc["city"].as<String>();
    }
    if (doc.containsKey("timezone")) {
      prefs.putString("tz", doc["timezone"].as<String>());
      g_savedTZ = doc["timezone"].as<String>();
    }
    if (doc.containsKey("currency")) {
      prefs.putString("currency", doc["currency"].as<String>());
      g_savedCurrency = doc["currency"].as<String>();
    }
    if (doc.containsKey("toptext")) {
      prefs.putString("toptext", doc["toptext"].as<String>());
      g_savedTopText = doc["toptext"].as<String>();
    }
    if (doc.containsKey("bottomtext")) {
      prefs.putString("bottomtext", doc["bottomtext"].as<String>());
      g_savedBottomText = doc["bottomtext"].as<String>();
    }
    
    prefs.end();
    
    response["success"] = true;
    Serial.println("💾 Settings saved via API");
  }
  
  String responseStr;
  serializeJson(response, responseStr);
  request->send(200, "application/json", responseStr);
}

// API Handler: WiFi Settings
void handleAPIWiFi(AsyncWebServerRequest* request, uint8_t *data, size_t len) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, data, len);
  
  StaticJsonDocument<128> response;
  
  if (error) {
    response["success"] = false;
    response["error"] = "Invalid JSON";
  } else {
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    
    if (ssid.length() > 0) {
      prefs.begin("sw", false);
      prefs.putString("ssid", ssid);
      prefs.putString("pass", password);
      prefs.end();
      
      response["success"] = true;
      Serial.println("🌐 WiFi settings updated via API");
      
      // Send response first, then reboot
      String responseStr;
      serializeJson(response, responseStr);
      request->send(200, "application/json", responseStr);
      
      delay(1000);
      ESP.restart();
      return;
    } else {
      response["success"] = false;
      response["error"] = "SSID required";
    }
  }
  
  String responseStr;
  serializeJson(response, responseStr);
  request->send(200, "application/json", responseStr);
}

// API Handler: Reset WiFi
void handleAPIResetWiFi(AsyncWebServerRequest* request) {
  prefs.begin("sw", false);
  prefs.remove("ssid");
  prefs.remove("pass");
  prefs.end();
  
  StaticJsonDocument<128> response;
  response["success"] = true;
  
  String responseStr;
  serializeJson(response, responseStr);
  request->send(200, "application/json", responseStr);
  
  Serial.println("🔄 WiFi settings reset via API");
  
  delay(1000);
  ESP.restart();
}

// API Handler: Reboot Device
void handleAPIReboot(AsyncWebServerRequest* request) {
  StaticJsonDocument<128> response;
  response["success"] = true;
  
  String responseStr;
  serializeJson(response, responseStr);
  request->send(200, "application/json", responseStr);
  
  Serial.println("🔄 Reboot requested via API");
  delay(1000);
  ESP.restart();
}

// Format uptime in human-readable format
String formatUptime(unsigned long milliseconds) {
  unsigned long seconds = milliseconds / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  String uptime = "";
  if (days > 0) {
    uptime += String(days) + "d ";
  }
  uptime += String(hours % 24) + "h ";
  uptime += String(minutes % 60) + "m ";
  uptime += String(seconds % 60) + "s";
  
  return uptime;
}

// Check WiFi connection and handle failures
void checkWiFiConnection() {
  if (millis() - lastWiFiCheck < WIFI_CHECK_INTERVAL) return;
  lastWiFiCheck = millis();
  
  if (WiFi.status() != WL_CONNECTED && !portalModeActive) {
    logPrintln("⚠️ WiFi connection lost - attempting to reconnect");
    
    // Try to reconnect first
    if (connectWiFiFromPrefs(5000)) {
      setupMDNS();  // Restart mDNS after reconnection
      return;
    }
    
    logPrintln("🚪 WiFi reconnection failed - starting portal mode");
    mdnsActive = false;
    startPortal();
    
    // Show portal screen if LVGL is ready
    if (lv_ready) {
      showportal_screen(g_apName);
    }
  }
}

// Try connecting from saved preferences
bool connectWiFiFromPrefs(uint32_t timeoutMs) {
  prefs.begin("sw", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.isEmpty()) return false;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);  // Enable automatic reconnection
  WiFi.setSleep(false);         // Disable WiFi sleep mode for stability
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("🌐 Connecting to %s", ssid.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250); Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("✅ WiFi OK: %s\n", WiFi.localIP().toString().c_str());
    setupMDNS();  // Start mDNS immediately after successful connection
    return true;
  }
  Serial.println("❌ WiFi failed");
  return false;
}

// Start AP + portal and routes
void startPortal() {
  portalModeActive = true;
  Serial.println("🚪 Starting Spark captive portal…");

  if (!SPIFFS.begin(true)) {
    Serial.println("⚠️ SPIFFS mount failed (formatting?)");
  }

  WiFi.mode(WIFI_AP);
  String apName = makeAPName();
  WiFi.softAP(apName.c_str());
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
IPAddress ip = WiFi.softAPIP();
Serial.printf("📡 AP SSID: %s  IP: %s\n", apName.c_str(), ip.toString().c_str());
Serial.printf("SPIFFS has %s.gz: %d  and %s: %d\n",
              PORTAL_FILE, SPIFFS.exists(String(PORTAL_FILE + String(".gz")).c_str()),
              PORTAL_FILE, SPIFFS.exists(PORTAL_FILE));


// Quick sanity: do we actually have the portal file?
Serial.printf("SPIFFS exists /index.html.gz: %d  /index.html: %d\n",
              SPIFFS.exists("/index.html.gz"), SPIFFS.exists("/index.html"));

  delay(200);

  dns.start(53, "*", apIP);  // simple captive-portal DNS

  setupWebRoutes();  // Setup all web routes
  server.begin();

// 👉 Show on-screen instructions
  //showPortalScreen(apName);
g_apName = apName;  // remember until LVGL ready

  Serial.printf("📶 AP: %s — connect and open http://192.168.4.1\n", apName.c_str());
}

// Setup all web server routes (both portal and dashboard)
void setupWebRoutes() {
  // Setup WebSocket for live logs
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  // -------- Portal routes (for AP mode) --------
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r){
    if (portalModeActive) {
      // Serve portal in AP mode
      String gz = String(PORTAL_FILE) + ".gz";  // "/STACKS_Wifi_Portal.html.gz"
      if (SPIFFS.exists(gz)) {
        AsyncWebServerResponse* resp = r->beginResponse(SPIFFS, gz, "text/html");
        resp->addHeader("Content-Encoding", "gzip");
        r->send(resp);
      } else if (SPIFFS.exists(PORTAL_FILE)) {
        r->send(SPIFFS, PORTAL_FILE, "text/html");
      } else {
        r->send(404, "text/plain", "Portal file missing");
      }
    } else {
      // Serve dashboard when connected to WiFi
      if (SPIFFS.exists("/dashboard.html")) {
        r->send(SPIFFS, "/dashboard.html", "text/html");
      } else {
        r->send(200, "text/html", generateDashboardFallback());
      }
    }
  });

// Friendly aliases (optional)
server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* r){ r->redirect("/"); });
server.on("/portal",     HTTP_GET, [](AsyncWebServerRequest* r){ r->redirect("/"); });
server.on("/dashboard",  HTTP_GET, [](AsyncWebServerRequest* r){
  if (SPIFFS.exists("/dashboard.html")) {
    r->send(SPIFFS, "/dashboard.html", "text/html");
  } else {
    r->send(200, "text/html", generateDashboardFallback());
  }
});

// Captive convenience: redirect any stray path to root (only in portal mode)
server.onNotFound([](AsyncWebServerRequest* r){ 
  if (portalModeActive) {
    r->redirect("/"); 
  } else {
    r->send(404, "text/plain", "Not Found");
  }
});

// Probe endpoint to verify the server is up
server.on("/ping", HTTP_GET, [](AsyncWebServerRequest* r){ r->send(200, "text/plain", "pong"); });

  // -------- API Endpoints --------
  
  // Device status API
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* r){
    handleAPIStatus(r);
  });
  
  // Settings API
  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest* r){
    // Will be handled by onRequestBody
  }, nullptr, [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t){
    handleAPISettings(req, data, len);
  });
  
  // WiFi API
  server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* r){
    // Will be handled by onRequestBody
  }, nullptr, [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t){
    handleAPIWiFi(req, data, len);
  });
  
  // Reset WiFi API
  server.on("/api/reset-wifi", HTTP_POST, [](AsyncWebServerRequest* r){
    handleAPIResetWiFi(r);
  });
  
  // Reboot API
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* r){
    handleAPIReboot(r);
  });

  // urlencoded form support (existing Matrix portal posts will hit this)
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *req){
    if (req->hasParam("ssid", true)) {
      // Start portal animation
      start_portal_save_animation();
      
      prefs.begin("sw", false);
      prefs.putString("ssid", req->arg("ssid"));
      prefs.putString("pass", req->arg("password"));
      prefs.putString("city", req->arg("city"));
      prefs.putString("tz",   req->arg("timezone"));
      prefs.putString("device", req->arg("device"));
      prefs.putString("currency", req->arg("currency"));  // 🌍 New currency support
      prefs.putString("theme", req->arg("theme"));        // 🎨 New theme support
      prefs.putString("toptext", req->arg("toptext"));    // 📝 Custom top message
      prefs.putString("bottomtext", req->arg("bottomtext")); // 📝 Custom bottom message
      prefs.end();
      Serial.println("💾 Saved portal fields (urlencoded).");
    }
    req->send(200, "text/plain", "OK");
  });

  // JSON body support
server.onRequestBody([](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t){
  if (req->url() == "/save") {
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
      Serial.printf("JSON parse error: %s\n", err.c_str());
      return;
    }

    // ✅ No casts; let ArduinoJson apply defaults
    String ssid   = doc["ssid"]     | "";
    String pass   = doc["password"] | "";
    String city   = doc["city"]     | "";
    String tz     = doc["timezone"] | "";
    String device = doc["device"]   | "";
    String currency = doc["currency"] | "USD";      // 🌍 Currency support
    String theme = doc["theme"] | "scroll";         // 🎨 Theme support
    String toptext = doc["toptext"] | "";           // 📝 Custom top message
    String bottomtext = doc["bottomtext"] | "";     // 📝 Custom bottom message

    if (ssid.length()) {
      // Start portal animation
      start_portal_save_animation();
      
      prefs.begin("sw", false);
      prefs.putString("ssid", ssid);
      prefs.putString("pass", pass);
      prefs.putString("city", city);
      prefs.putString("tz",   tz);
      prefs.putString("device", device);
      prefs.putString("currency", currency);    // 🌍 Save currency
      prefs.putString("theme", theme);          // 🎨 Save theme
      prefs.putString("toptext", toptext);      // 📝 Save custom top
      prefs.putString("bottomtext", bottomtext); // 📝 Save custom bottom
      prefs.end();
      Serial.println("💾 Saved portal fields (JSON).");
    }
  }
});

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Rebooting…");
    delay(250);
    ESP.restart();
  });
}



// Map Open-Meteo weather codes (Matrix-style mapping)
static String mapWeatherCode(int code) {
  if (code == 0) return "Sunny";
  if (code == 1) return "Mostly Sunny";
  if (code == 2) return "Partly Cloudy";
  if (code == 3) return "Cloudy";
  if (code >= 45 && code <= 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 85 && code <= 86) return "Snow Showers";
  if (code >= 95 && code <= 99) return "Thunderstorm";
  return "Unknown";
}

// Load saved City & TZ from portal prefs
static void load_city_tz_from_prefs() {
  prefs.begin("sw", true);
  g_savedCity = prefs.getString("city", "");
  g_savedTZ   = prefs.getString("tz", "");
  g_savedTopText = prefs.getString("toptext", "");       // 📝 Custom top row message
  g_savedBottomText = prefs.getString("bottomtext", ""); // 📝 Custom bottom row message
  g_savedCurrency = prefs.getString("currency", "USD");  // 🌍 Default to USD
  g_savedTheme = prefs.getString("theme", "scroll");     // 🎨 Default to scroll
  prefs.end();
  Serial.printf("🌆 City: %s  |  TZ: %s\n", g_savedCity.c_str(), g_savedTZ.c_str());
  Serial.printf("📝 Custom Messages - Top: '%s'  Bottom: '%s'\n", g_savedTopText.c_str(), g_savedBottomText.c_str());
  Serial.printf("🌍 Currency: %s  |  Theme: %s\n", g_savedCurrency.c_str(), g_savedTheme.c_str());
}

// Apply timezone (Olson string) and start NTP
static void setup_tz() {
  if (g_savedTZ.length()) {
    configTzTime(g_savedTZ.c_str(), "pool.ntp.org", "time.nist.gov", "time.google.com");
    Serial.printf("🕒 TZ set to %s\n", g_savedTZ.c_str());
  }
}

// Format local time like 9:07PM (no leading zero)
static String now_time_12h() {
  struct tm tm;
  if (!getLocalTime(&tm)) return String("--:--");
  char buf[16];
  strftime(buf, sizeof(buf), "%I:%M%p", &tm);
  if (buf[0] == '0') memmove(buf, buf + 1, strlen(buf));   // drop leading 0
  return String(buf);
}

// Is the RTC actually set yet?
static bool timeIsValid() {
  struct tm tm;
  if (!getLocalTime(&tm)) return false;
  return tm.tm_year >= (2020 - 1900);  // >= 2020
}


// City -> lat/lon using Nominatim
static bool geocode_city(String city, float &lat, float &lon,
                         String &outCity, String &outRegion, String &outCountry) {
  if (!city.length()) return false;

  WiFiClientSecure client;
  client.setInsecure();  // baby-step: skip cert validation for now

  HTTPClient http;
  String url = "https://nominatim.openstreetmap.org/search?format=json&addressdetails=1&limit=1&city=" + city;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent("STACKSWORTH-Spark/0.0.3");
  if (!http.begin(client, url)) return false;

  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, http.getString());
  http.end();
  if (err || doc.isNull() || doc.size() == 0) return false;

  String latStr = doc[0]["lat"] | "";
  String lonStr = doc[0]["lon"] | "";
  lat = latStr.toFloat();
  lon = lonStr.toFloat();

  JsonObject addr = doc[0]["address"];
  outCity    = (const char*)(addr["city"]    | addr["town"] | addr["village"] | addr["hamlet"] | city.c_str());
  outRegion  = (const char*)(addr["state"]   | addr["province"] | "");
  outCountry = (const char*)(addr["country"] | "");

  Serial.printf("📍 Geocoded: %s, %s, %s  ->  %.5f, %.5f\n",
                outCity.c_str(), outRegion.c_str(), outCountry.c_str(), lat, lon);
  return true;
}

// Current weather via Open-Meteo
static bool fetch_weather(float lat, float lon, int &tempC, String &condition) {
  if (lat == 0 && lon == 0) return false;
  
  // Safety: Check WiFi connection before making request
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ Weather fetch: WiFi not connected");
    return false;
  }
  
  // Safety: Feed watchdog before network operation
  esp_task_wdt_reset();

  WiFiClientSecure client;
  client.setInsecure();  // baby-step
  client.setTimeout(5000);  // 5 second timeout to prevent hanging

  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
               "&longitude=" + String(lon, 6) +
               "&current=temperature_2m,weather_code&timezone=auto";
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(5000);  // HTTP timeout
  
  if (!http.begin(client, url)) {
    Serial.println("❌ Weather fetch: HTTP begin failed");
    return false;
  }

  Serial.println("🌐 Fetching weather data...");
  int code = http.GET();
  if (code != 200) { 
    Serial.printf("❌ Weather fetch: HTTP %d\n", code);
    http.end(); 
    return false; 
  }

  String payload = http.getString();
  http.end();
  
  // Safety: Feed watchdog after network operation
  esp_task_wdt_reset();
  
  if (!payload.length()) {
    Serial.println("❌ Weather fetch: Empty response");
    return false;
  }

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, payload)) {
    Serial.println("❌ Weather fetch: JSON parse failed");
    return false;
  }

  float t = doc["current"]["temperature_2m"] | NAN;
  int   w = doc["current"]["weather_code"]   | -1;
  if (isnan(t) || w < 0) {
    Serial.println("❌ Weather fetch: Invalid data in response");
    return false;
  }

  tempC = (int)t;
  condition = mapWeatherCode(w);
  Serial.printf("✅ Weather: %d°C, %s (code %d)\n", tempC, condition.c_str(), w);
  return true;
}





 // ---- pretty price formatting: $12,345.67 ----
static String fmt_with_commas(unsigned long v) {
  String s = String(v);
  int insert = s.length() - 3;
  while (insert > 0) {
    s = s.substring(0, insert) + "," + s.substring(insert);
    insert -= 3;
  }
  return s;
}

static void format_price_usd(float value, char* out, size_t outlen, const char* prefix="$") {
  if (value < 0) value = 0;
  unsigned long whole = (unsigned long)value;
  int cents = (int)((value - (float)whole) * 100.0f + 0.5f);
  if (cents >= 100) { whole++; cents -= 100; }
  String s = String(prefix) + fmt_with_commas(whole) + "." + (cents < 10 ? "0" : "") + String(cents);
  s.toCharArray(out, outlen);
}


// ---- Mid-strip cache so we only paint when all parts are ready ----
static float g_hashrateEh = NAN;
static float g_diffPct    = NAN;
static int   g_diffDays   = -1;

static float g_mcapUsd    = NAN;
static float g_circBtc    = NAN;
static float g_athUsd     = NAN;
static int   g_athDays    = -1;

static inline bool mid_ready() {
  return isfinite(g_hashrateEh) && isfinite(g_diffPct) && g_diffDays >= 0 &&
         isfinite(g_mcapUsd)   && isfinite(g_circBtc) && isfinite(g_athUsd) && g_athDays >= 0;
}

// ===== Smart Cache System (Matrix-style) =====

// Save successful API values to preferences for fallback use
void saveDisplayCache() {
  prefs.begin("cache", false);  // write mode
  prefs.putString("lastPrice", lastPrice);
  prefs.putString("lastFee", lastFee);
  prefs.putString("lastBlockHeight", lastBlockHeight);
  prefs.putString("lastMiner", lastMiner);
  
  // Save cache timestamp
  prefs.putULong("cacheTime", millis());
  prefs.end();
  Serial.println("💾 Display cache saved");
}

// Load cached values on boot (fallback during API failures)
void loadDisplayCache() {
  prefs.begin("cache", true);  // read mode
  
  String cachedPrice = prefs.getString("lastPrice", "Loading…");
  String cachedFee = prefs.getString("lastFee", "Loading…");
  String cachedHeight = prefs.getString("lastBlockHeight", "Loading…");
  String cachedMiner = prefs.getString("lastMiner", "Loading…");
  
  unsigned long cacheAge = millis() - prefs.getULong("cacheTime", 0);
  prefs.end();
  
  // Use cached values as initial display (prevents blank screens)
  lastPrice = cachedPrice;
  lastFee = cachedFee;
  lastBlockHeight = cachedHeight;
  lastMiner = cachedMiner;
  
  // Update UI if labels exist
  if (priceValueLabel) lv_label_set_text(priceValueLabel, lastPrice.c_str());
  if (feeValueLabel) lv_label_set_text(feeValueLabel, lastFee.c_str());
  if (blockValueLabel) lv_label_set_text(blockValueLabel, lastBlockHeight.c_str());
  if (solvedByValueLabel) lv_label_set_text(solvedByValueLabel, lastMiner.c_str());
  
  Serial.printf("📦 Cache loaded (age: %lu ms) - Price: %s, Fee: %s, Height: %s, Miner: %s\n", 
                cacheAge, cachedPrice.c_str(), cachedFee.c_str(), cachedHeight.c_str(), cachedMiner.c_str());
}

static void paint_mid_if_ready() {
  if (mid_ready()) {
    // last arg fromAthPct is unused in your UI for now
    ui_update_mid_metrics(g_hashrateEh, g_diffPct, g_diffDays, g_mcapUsd, g_circBtc, g_athUsd, g_athDays, 0.0f);
  }
}


 
 // Styles
 String title = "STACKSWORTH Spark – Satscape v0.0.2";
 lv_style_t widgetStyle;
 lv_style_t widget2Style;
 lv_style_t glowStyle;
 
 lv_style_t orangeStyle, greenStyle;
 lv_style_t blueStyle, yellowStyle;
 
 
// ===== LVGL portal screen bits =====
lv_obj_t* portalScreen = nullptr;
lv_obj_t* portalNetLabel = nullptr;
lv_obj_t* portalUrlLabel = nullptr;
lv_obj_t* portalCard = nullptr;
lv_obj_t* portalStatusLabel = nullptr;
lv_timer_t* portalAnimTimer = nullptr;





 
 struct MinerTag {
   const char* tag;
   const char* name;
 };
 
 const MinerTag knownTags[] = {
   { "f2pool", "F2Pool" },
   { "antpool", "AntPool" },
   { "viabtc", "ViaBTC" },
   { "poolin", "Poolin" },
   { "btccom", "BTC.com" },
   { "binance", "Binance Pool" },
   { "carbon", "Carbon Negative" },
   { "slush", "Slush Pool" },
   { "braiins", "Braiins Pool" },
   { "foundry", "Foundry USA" },
   { "ocean", "Ocean Pool" },
   { "mara", "Marathon" },
   { "marathon", "Marathon" },
   { "luxor", "Luxor" },
   { "ultimus", "ULTIMUSPOOL" },
   { "novablock", "NovaBlock" },
   { "sigma", "SigmaPool" },
   { "spider", "SpiderPool" },
   { "tera", "TERA Pool" },
   { "okex", "OKEx Pool" },
   { "kucoin", "KuCoin Pool" },
   { "sbi", "SBI Crypto" },
   { "btctop", "BTC.TOP" },
   { "emcd", "EMCD Pool" },
   { "secpool", "SECPOOL" },
   { "hz", "HZ Pool" },
   { "solo.ckpool", "Solo CKPool" },
   { "solopool", "Solo Pool" },
   { "solo", "Solo Miner" },           // Catch-all fallback for anything solo-ish
   { "bitaxe", "Bitaxe Solo Miner" },
   { "node.pw", "Node.PW" },
   { "/axe/", "Bitaxe Solo Miner" }            
 };
 
 String hexToAscii(const String& hex) {
   String ascii = "";
   for (unsigned int i = 0; i < hex.length(); i += 2) {
     String byteString = hex.substring(i, i + 2);
     char c = (char) strtol(byteString.c_str(), nullptr, 16);
     if (isPrintable(c)) {
       ascii += c;
     }
   }
   return ascii;
 }
 
 
 String identifyMiner(String scriptSig) {
   scriptSig.toLowerCase(); // Normalize for comparison
 
   for (const auto& tag : knownTags) {
     if (scriptSig.indexOf(tag.tag) != -1) {
       return tag.name;
     }
   }
 
   // Optional: return partial string for unknowns (dev mode)
   if (scriptSig.length() > 0) {
     int maxLen = 24;
     String preview = scriptSig.substring(0, min((int)scriptSig.length(), maxLen));
     return "Unknown (" + preview + ")";
   }
 
   return "Unknown";
 }
 
 

 //int lastBlockHeight = 0;



//***Create Screen

// Portal save animation states
enum PortalAnimState { IDLE, SAVING, SAVED, REBOOTING };
static PortalAnimState animState = IDLE;
static unsigned long animStartTime = 0;

// Portal animation timer callback - simplified to prevent crashes
static void portal_anim_timer_cb(lv_timer_t* timer) {
  if (!portalCard || !portalStatusLabel) return;
  
  unsigned long elapsed = millis() - animStartTime;
  
  switch(animState) {
    case SAVING:
      // Simple solid orange (0-800ms)
      if (elapsed < 800) {
        lv_obj_set_style_border_color(portalCard, lv_color_hex(0xFF8800), 0);
        lv_obj_set_style_border_width(portalCard, 3, 0);
      } else {
        animState = SAVED;
        animStartTime = millis();
        lv_label_set_text(portalStatusLabel, "✅ Saved!");
      }
      break;
      
    case SAVED:
      // Simple solid green for 800ms
      if (elapsed < 800) {
        lv_obj_set_style_border_color(portalCard, lv_color_hex(0x00FF88), 0);
        lv_obj_set_style_border_width(portalCard, 3, 0);
      } else {
        animState = REBOOTING;
        animStartTime = millis();
        lv_label_set_text(portalStatusLabel, "🔄 Rebooting...");
      }
      break;
      
    case REBOOTING:
      // Simple solid red (800ms)
      if (elapsed < 800) {
        lv_obj_set_style_border_color(portalCard, lv_color_hex(0xFF4444), 0);
        lv_obj_set_style_border_width(portalCard, 4, 0);
      } else {
        // Animation complete - stop timer
        lv_timer_del(portalAnimTimer);
        portalAnimTimer = nullptr;
        // ESP will reboot shortly
      }
      break;
      
    default:
      break;
  }
}

// Start portal save animation
void start_portal_save_animation() {
  if (!portalCard || !portalStatusLabel) return;
  
  animState = SAVING;
  animStartTime = millis();
  
  // Update status text
  lv_label_set_text(portalStatusLabel, "Saving...");
  
  // Start animation timer (100ms intervals for smoother animation)
  if (!portalAnimTimer) {
    portalAnimTimer = lv_timer_create(portal_anim_timer_cb, 100, nullptr);
  }
}


lv_obj_t* create_portal_screen(const String& apName) {
  // Root screen - FORCE BLACK EVERYTHING
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_size(scr, 800, 480);  // Force full screen size
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(scr, 0, 0);  // No padding
  lv_obj_set_style_border_width(scr, 0, 0);  // No border

  // Title labels
  lv_obj_t* stacksworthLabel = lv_label_create(scr);
  lv_label_set_text(stacksworthLabel, "STACKSWORTH");
  lv_obj_set_style_text_color(stacksworthLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(stacksworthLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(stacksworthLabel, LV_ALIGN_TOP_LEFT, 25, 10);

  lv_obj_t* sparkLabel = lv_label_create(scr);
  lv_label_set_text(sparkLabel, "// SPARK");
  lv_obj_set_style_text_color(sparkLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(sparkLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(sparkLabel, LV_ALIGN_TOP_LEFT, 190, 10);

  lv_obj_t* setupLabel = lv_label_create(scr);
  lv_label_set_text(setupLabel, "SETUP");
  lv_obj_set_style_text_color(setupLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(setupLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(setupLabel, LV_ALIGN_TOP_RIGHT, -25, 10);

  lv_obj_t* inf = lv_label_create(scr);
  lv_label_set_text(inf, "SPARK v1.1.1");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -60, 0);

  // Manual card - NO THEME SYSTEM
  portalCard = lv_obj_create(scr);
  lv_obj_set_size(portalCard, 600, 300);
  lv_obj_align(portalCard, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(portalCard, lv_color_hex(0x1A1A1A), 0);
  lv_obj_set_style_bg_opa(portalCard, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(portalCard, lv_color_hex(0x00F5FF), 0);
  lv_obj_set_style_border_width(portalCard, 2, 0);
  lv_obj_set_style_radius(portalCard, 12, 0);
  lv_obj_set_style_pad_all(portalCard, 20, 0);

  // Title
  lv_obj_t* title = lv_label_create(portalCard);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x00F5FF), 0);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(title, "WIFI SETUP PORTAL");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Network name
  lv_obj_t* networkLabel = lv_label_create(portalCard);
  lv_obj_set_style_text_font(networkLabel, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(networkLabel, lv_color_white(), 0);
  lv_obj_set_style_text_align(networkLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(networkLabel, "Connect to Wi-Fi Network:");
  lv_obj_align(networkLabel, LV_ALIGN_CENTER, 0, -60);

  lv_obj_t* apNameLabel = lv_label_create(portalCard);
  lv_obj_set_style_text_font(apNameLabel, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(apNameLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_align(apNameLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text_fmt(apNameLabel, "%s", apName.c_str());
  lv_obj_align(apNameLabel, LV_ALIGN_CENTER, 0, -20);

  // Instructions
  lv_obj_t* instructions = lv_label_create(portalCard);
  lv_obj_set_style_text_font(instructions, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(instructions, lv_color_white(), 0);
  lv_obj_set_style_text_align(instructions, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(instructions, "Setup portal should open automatically\nor manually visit: http://192.168.4.1");
  lv_obj_align(instructions, LV_ALIGN_CENTER, 0, 30);

  // Status
  portalStatusLabel = lv_label_create(portalCard);
  lv_obj_set_style_text_font(portalStatusLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(portalStatusLabel, lv_color_hex(0x00F5FF), 0);
  lv_obj_set_style_text_align(portalStatusLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(portalStatusLabel, "Waiting for setup...");
  lv_obj_align(portalStatusLabel, LV_ALIGN_BOTTOM_MID, 0, -10);


 

  return scr;
}


void showportal_screen(const String& apName) {
  if (!lv_ready) return;

  // If screen already created, just reload it
  if (portalScreen) {
    lv_scr_load(portalScreen);
    return;
  }

  // Create and load the portal screen
  portalScreen = create_portal_screen(apName);
  lv_scr_load(portalScreen);
}

// ===== SatoNak API Functions =====

// Fetch Bitcoin price from SatoNak API with fallback
bool fetchPriceFromSatoNak() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String currentFiat = getCurrentFiatCode();
  String url = satonakUrl(SATONAK_PRICE, currentFiat.c_str());
  Serial.print("🌐 SatoNak Price GET: "); Serial.println(url);

  HTTPClient http;
  http.setTimeout(3000);
  http.setConnectTimeout(2000); 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(url)) return false;

  int rc = http.GET();
  if (rc != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();

  // Try plain text format first (e.g., "90768.00")
  payload.trim();  // Remove any whitespace
  double price = payload.toDouble();
  
  // If plain text parsing fails, try JSON format
  if (price <= 0.0) {
    DynamicJsonDocument doc(1536);
    DeserializationError e = deserializeJson(doc, payload);
    if (e) return false;

    // Get price for current fiat
    String key = currentFiat; key.toLowerCase();
    
    if (doc.containsKey("value")) {
      price = doc["value"] | 0.0;
    }
  }
  
  Serial.printf("🔍 SatoNak payload: '%s' -> price: %.2f\n", payload.c_str(), price);
  
  Serial.printf("🔍 SatoNak payload: '%s' -> price: %.2f\n", payload.c_str(), price);
  
  if (price <= 0.0) return false;

  // Update price display
  String symbol = getCurrencySymbol();
  char priceText[32];
  if (currentFiat == "USD") {
    format_price_usd(price, priceText, sizeof(priceText));
  } else {
    format_price_usd(price, priceText, sizeof(priceText), symbol.c_str());
  }
  
  lastPrice = priceText;
  if (priceValueLabel) lv_label_set_text(priceValueLabel, priceText);
  
  // Calculate sats per currency
  int sats = (price > 0.0f) ? (int)(100000000.0f / price) : 0;
  String satsLine = sats ? String(sats) + " SATS / " + symbol + "1 " + currentFiat : String("… SATS / ") + symbol + "1 " + currentFiat;
  
  // Update cache
  auto& p = Cache::price;
  p.usdPretty = priceText;
  p.satsUsd = satsLine;
  
  Serial.printf("✅ SatoNak Price: %s | Sats/%s: %d\n", priceText, currentFiat.c_str(), sats);
  saveDisplayCache();  // 💾 Save to cache on success
  return true;
}

// Fetch block height from SatoNak API
bool fetchHeightFromSatoNak() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = String(SATONAK_BASE) + String(SATONAK_HEIGHT);
  HTTPClient http;
  http.setTimeout(3000);
  
  if (!http.begin(url)) return false;
  
  int rc = http.GET();
  if (rc != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();
  
  payload.trim();
  int newHeight = payload.toInt();
  if (newHeight > 0) {
    lastBlockHeight = String(newHeight);
    if (blockValueLabel) lv_label_set_text(blockValueLabel, lastBlockHeight.c_str());
    ui_update_blocks_to_million(newHeight);
    Cache::block.height = String(newHeight);
    
    Serial.printf("✅ SatoNak Height: %d\n", newHeight);
    saveDisplayCache();  // 💾 Save to cache on success
    return true;
  }
  return false;
}

// Fetch miner from SatoNak API  
bool fetchMinerFromSatoNak() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = String(SATONAK_BASE) + String(SATONAK_MINER);
  HTTPClient http;
  http.setTimeout(3000);
  
  if (!http.begin(url)) return false;
  
  int rc = http.GET();
  if (rc != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();
  
  payload.trim();
  if (payload.length() > 0 && payload.length() < 32) {
    lastMiner = payload;
    if (solvedByValueLabel) lv_label_set_text(solvedByValueLabel, payload.c_str());
    Cache::block.miner = payload;
    
    Serial.printf("✅ SatoNak Miner: %s\n", payload.c_str());
    saveDisplayCache();  // 💾 Save to cache on success
    return true;
  }
  return false;
}

// Fetch fees from SatoNak API
bool fetchFeeFromSatoNak() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = String(SATONAK_BASE) + "/api/fee";
  HTTPClient http;
  http.setTimeout(3000);
  
  if (!http.begin(url)) return false;
  
  int rc = http.GET();
  if (rc != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();
  
  payload.trim();
  
  // Handle both plain text and JSON responses
  int fee = 0;
  if (payload.length() > 0 && payload.length() < 8 && isdigit(payload.charAt(0))) {
    fee = payload.toInt();
  } else {
    DynamicJsonDocument doc(512);
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      fee = doc["value"] | 0;
    }
  }
  
  if (fee > 0 && fee < 1000) {
    char feeText[16];
    snprintf(feeText, sizeof(feeText), "%d sat/vB", fee);
    lastFee = feeText;
    if (feeValueLabel) lv_label_set_text(feeValueLabel, feeText);
    
    // Update fee badges (use fee as high, estimate others)
    int high = fee;
    int med = (int)(fee * 0.8f);
    int low = (int)(fee * 0.5f);
    
    auto& f = Cache::fees;
    f.low = low; 
    f.med = med; 
    f.high = high;
    
    ui_update_fee_badges_lmh(low, med, high);
    
    Serial.printf("✅ SatoNak Fee: %d sat/vB\n", fee);
    saveDisplayCache();  // 💾 Save to cache on success
    return true;
  }
  return false;
}


 
 void fetchBitcoinData() {
  if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // Step 1: Fetch Bitcoin Price - Try SatoNak first, fallback to CoinGecko
      if (!fetchPriceFromSatoNak()) {
        Serial.println("⚠️ SatoNak price failed, trying CoinGecko fallback");
        
        http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd,cad&include_24hr_change=true");
        int httpCode = http.GET();
        if (httpCode == 200) {
            String payload = http.getString();
            DynamicJsonDocument doc(512);
            deserializeJson(doc, payload);

            float usd = doc["bitcoin"]["usd"] | 0.0f;
            float cad = doc["bitcoin"]["cad"] | 0.0f;

            // USD price (pretty)
            char usdBuf[32];
            format_price_usd(usd, usdBuf, sizeof(usdBuf));  // → "$69,420.13"
            lastPrice = usdBuf;
            if (priceValueLabel) lv_label_set_text(priceValueLabel, usdBuf);
            Serial.print("💰 Bitcoin Price (USD): "); Serial.println(usdBuf);

            // Build CAD line and SATS/$ lines
            char cadCore[32];
            format_price_usd(cad, cadCore, sizeof(cadCore));     // "$69,420.13"
            String cadLine = String("CAD ") + cadCore;

            int sats_usd = (usd > 0.0f) ? (int)(100000000.0f / usd) : 0;
            int sats_cad = (cad > 0.0f) ? (int)(100000000.0f / cad) : 0;
            String satsUsdLine = sats_usd ? String(sats_usd) + " SATS / $1 USD" : String("… SATS / $1 USD");
            String satsCadLine = sats_cad ? String(sats_cad) + " SATS / $1 CAD" : String("… SATS / $1 CAD");

            // 24h change
            float changePct = doc["bitcoin"]["usd_24h_change"] | 0.0f;
            Serial.printf("[price] 24h change (usd): %+.2f%%\n", changePct);

            // ---- WRITE TO SHARED CACHE (so other screens hydrate instantly)
            {
              auto& p = Cache::price;
              p.usdPretty   = usdBuf;        // "$69,420.13"
              p.cadLine     = cadLine;       // "CAD $92,314.55"
              p.satsUsd     = satsUsdLine;   // "1450 SATS / $1 USD"
              p.satsCad     = satsCadLine;   // "1320 SATS / $1 CAD"
              p.changePct   = changePct;
              p.changeValid = true;
            }

            // Optional: still cache+paint the local per-screen strings
            ui_cache_price_aux(cadLine, satsUsdLine, satsCadLine);

            // (These direct sets are now redundant but harmless)
            if (priceCadLabel) lv_label_set_text(priceCadLabel, cadLine.c_str());
            if (satsUsdLabel)  lv_label_set_text(satsUsdLabel,  satsUsdLine.c_str());
            if (satsCadLabel)  lv_label_set_text(satsCadLabel,  satsCadLine.c_str());

            Serial.print("💵 Bitcoin Price (CAD): "); Serial.println(cadLine);

            // Update the pill (also reads from the same changePct we stored)
            ui_update_change_pill(changePct);

            // Push to footer chart (USD for now)
            if (priceChart && priceSeries) {
              lv_chart_set_next_value(priceChart, priceSeries, (int)usd);
              lv_chart_refresh(priceChart);
            }

        } else {
            Serial.printf("❌ Failed to fetch Bitcoin price. HTTP=%d\n", httpCode);
        }
        http.end();
      }

     // Step 2: Fetch Block Height - Try SatoNak first
     if (!fetchHeightFromSatoNak()) {
        Serial.println("⚠️ SatoNak height failed, trying mempool.space fallback");
        
        http.begin("https://mempool.space/api/blocks/tip/height");
        int httpCode = http.GET();
        if (httpCode == 200) {
          String blockHeightStr = http.getString();

          lastBlockHeight = blockHeightStr;
          if (blockValueLabel) lv_label_set_text(blockValueLabel, lastBlockHeight.c_str());

          long h = blockHeightStr.toInt();
          if (h > 0) ui_update_blocks_to_million(h);

          // 🔒 CACHE → shared data store
          Cache::block.height = blockHeightStr;

          Serial.printf("📏 Block Height: %ld\n", h);
        } else {
          Serial.println("❌ Failed to fetch block height.");
        }
        http.end();
     }

     // Step 3: Fetch Fee Estimates - Try SatoNak first
     if (!fetchFeeFromSatoNak()) {
        Serial.println("⚠️ SatoNak fee failed, trying mempool.space fallback");
        
        http.begin("https://mempool.space/api/v1/fees/recommended");
        int httpCode = http.GET();
        if (httpCode == 200) {
          String payload = http.getString();
          DynamicJsonDocument doc(512);
          deserializeJson(doc, payload);

          // Map mempool fields → LOW/MED/HIGH
          int high = doc["fastestFee"] | 0;                       // HIGH (red)
          int med  = doc["halfHourFee"] | high;                   // MED  (yellow)
          int low  = doc["economyFee"] | (doc["hourFee"] | med);  // LOW  (green)

          // Keep my big label as-is (currently fastest/high)
          char feeFormatted[16];
          snprintf(feeFormatted, sizeof(feeFormatted), "%d sat/vB", high);
          lastFee = feeFormatted;
          if (feeValueLabel) lv_label_set_text(feeValueLabel, feeFormatted);

          // 🔒 CACHE → shared data store
          {
            auto& f = Cache::fees;
            f.low = low; 
            f.med = med; 
            f.high = high;
          }

          // NEW: update the three badges
          ui_update_fee_badges_lmh(low, med, high);

          Serial.printf("⚡ Fees — LOW:%d  MED:%d  HIGH:%d (big:%s)\n", low, med, high, feeFormatted);
        } else {
          Serial.printf("❌ Failed to fetch fee estimates. HTTP=%d\n", httpCode);
        }
        http.end();
      }

      // Step 4: Fetch Miner Information - Try SatoNak first  
      if (!fetchMinerFromSatoNak()) {
        Serial.println("⚠️ SatoNak miner failed, trying mempool.space fallback");
        
        // Get the block hash
        String blockHash = "";
       
        http.begin("https://mempool.space/api/blocks/tip/hash");
        int httpCode = http.GET();
        if (httpCode == 200) {
            blockHash = http.getString();
            Serial.println("🔗 Block hash: " + blockHash);
        } else {
            Serial.println("❌ Failed to fetch block hash!");
        }
        http.end();

      // After obtaining blockHash
      if (blockHash.length()) {
  String blockUrl = "https://mempool.space/api/block/" + blockHash;
  http.begin(blockUrl);
  httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument bdoc(2048);
    DeserializationError berr = deserializeJson(bdoc, payload);
    if (!berr) {
      uint32_t ts = bdoc["timestamp"] | 0;
      if (ts > 0) {
        ui_update_block_age_from_unix(ts);      // paint pill
        // 🔒 CACHE → shared data store
        Cache::block.ts = ts;
      }
    } else {
      Serial.printf("❌ Block JSON parse: %s\n", berr.c_str());
    }
  } else {
    Serial.printf("❌ Block fetch failed: %d\n", httpCode);
  }
  http.end();
}



      

      if (blockHash != "") {
          // Get the coinbase TXID directly
          String cbTxUrl = "https://mempool.space/api/block/" + blockHash + "/txid/0";
          Serial.println("🌐 Requesting Coinbase TXID: " + cbTxUrl);
          http.begin(cbTxUrl);
          httpCode = http.GET();

          String coinbaseTxId = "";
          if (httpCode == 200) {
              coinbaseTxId = http.getString();
              Serial.print("🔑 Coinbase TXID: ");
              Serial.println(coinbaseTxId);
          } else {
              Serial.println("❌ Failed to fetch coinbase TXID.");
          }
          http.end();

          if (coinbaseTxId != "") {
              // Fetch the coinbase transaction details
              String txUrl = "https://mempool.space/api/tx/" + coinbaseTxId;
              Serial.println("🌐 Fetching coinbase TX: " + txUrl);
              http.begin(txUrl);
              httpCode = http.GET();

              if (httpCode == 200) {
                  String txPayload = http.getString();
                  Serial.println("🧪 Coinbase TX JSON:");
                  Serial.println(txPayload);

                  DynamicJsonDocument txDoc(8192);
                  DeserializationError txError = deserializeJson(txDoc, txPayload);

                  if (txError) {
                      Serial.print("❌ TX JSON Deserialization failed: ");
                      Serial.println(txError.c_str());
                  } else {
                      const char* scriptSig = txDoc["vin"][0]["scriptsig"];
                      String miner = "Unknown";

                      if (scriptSig) {
                          String hexStr = String(scriptSig);
                          Serial.print("📦 scriptsig: ");
                          Serial.println(hexStr);

                          String decoded = hexToAscii(hexStr);
                          Serial.print("🔤 Decoded ASCII: ");
                          Serial.println(decoded);

                          
                          
                          miner = identifyMiner(decoded);
                          lastMiner = miner; // Store last miner to be displayed so no blank space is present
                      } else {
                          Serial.println("⚠️ scriptsig not found!");
                      }

                      lv_label_set_text(solvedByValueLabel, miner.c_str()); // Update miner label
                      Serial.printf("✅ Solved by: %s\n", miner.c_str());

                     // 🔒 CACHE → shared data store
                     Cache::block.miner = miner; 
                      
                  }
              } else {
                  Serial.printf("❌ Failed to fetch coinbase TX: %d\n", httpCode);
                  lv_label_set_text(solvedByValueLabel, "Error");
              }
              http.end();
          } else {
              Serial.println("⚠️ Coinbase TXID was empty. Aborting miner lookup.");
          }
      } else {
          Serial.println("⚠️ Invalid block hash. Skipping coinbase lookup.");
      }
      } // End of miner fetch from SatoNak fallback
  } else {
      Serial.println("❌ WiFi not connected!");
  }
}

static void fetch_hashrate_and_diff() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;

  // --- Hashrate (EH/s) from 3-day series; take latest point ---
  double latestEH = NAN;
  {
    http.begin("https://mempool.space/api/v1/mining/hashrate/3d");
    int code = http.GET();
    if (code == 200) {
      String body = http.getString();
      DynamicJsonDocument doc(8192);
      if (deserializeJson(doc, body) == DeserializationError::Ok && doc.is<JsonArray>()) {
        JsonArray arr = doc.as<JsonArray>();
        if (!arr.isNull() && arr.size() > 0) {
          JsonObject last = arr[arr.size() - 1];
          // endpoint uses avgHashrate (H/s); normalize to EH/s
          double h = last["avgHashrate"] | last["avgHashRate"] | last["hashrate"] | 0.0;
          latestEH = h / 1e18;
        }
      }
    }
    http.end();
  }

  // --- Difficulty epoch info ---
  double diffPct = NAN;
  int    daysAgo = -1;
  {
    http.begin("https://mempool.space/api/v1/difficulty-adjustment");
    int code = http.GET();
    if (code == 200) {
      String body = http.getString();
      DynamicJsonDocument doc(4096);
      if (deserializeJson(doc, body) == DeserializationError::Ok && doc.is<JsonObject>()) {
        JsonObject o = doc.as<JsonObject>();
        diffPct = o["difficultyChange"] | 0.0;     // % change this epoch (running estimate)
        int remainingBlocks = o["remainingBlocks"] | -1;
        double timeAvg = o["timeAvg"] | 600.0;     // avg sec per block this epoch
        if (remainingBlocks >= 0) {
          int mined = 2016 - remainingBlocks;
          daysAgo = (int)round((mined * timeAvg) / 86400.0);
        }
      }
    }
    http.end();
  }

  if (isfinite(latestEH)) g_hashrateEh = (float)latestEH;
  if (isfinite(diffPct))  g_diffPct    = (float)diffPct;
  if (daysAgo >= 0)       g_diffDays   = daysAgo;

  paint_mid_if_ready();
}



static time_t parse_iso_ymd_utc(const String& iso) {
  // Expect "YYYY-MM-DD..." (CoinGecko ath_date)
  if (iso.length() < 10) return 0;
  int y = iso.substring(0,4).toInt();
  int m = iso.substring(5,7).toInt();
  int d = iso.substring(8,10).toInt();
  struct tm t = {0};
  t.tm_year = y - 1900; t.tm_mon = m - 1; t.tm_mday = d;
  // mktime uses local TZ; for day differences that’s fine.
  return mktime(&t);
}

static void fetch_market_meta() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/coins/bitcoin?localization=false&market_data=true&sparkline=false");
  int code = http.GET();
  if (code == 200) {
    String body = http.getString();
    DynamicJsonDocument doc(16384);
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      JsonObject md = doc["market_data"];
      double mcap = md["market_cap"]["usd"] | 0.0;
      double circ = md["circulating_supply"] | 0.0;
      double ath  = md["ath"]["usd"] | 0.0;
      const char* athDate = md["ath_date"]["usd"] | "";
      if (mcap > 0) g_mcapUsd = (float)mcap;
      if (circ > 0) g_circBtc = (float)circ;
      if (ath  > 0) g_athUsd  = (float)ath;
      if (athDate && *athDate) {
        time_t then = parse_iso_ymd_utc(String(athDate));
        time_t now  = time(nullptr);
        if (then > 0 && now > then) g_athDays = (int)((now - then) / 86400);
      }
    }
  }
  http.end();

  paint_mid_if_ready();
}



static void fetch_block_intervals_footer() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin("https://mempool.space/api/blocks");
  int code = http.GET();
  if (code != 200) { http.end(); return; }
  String body = http.getString();
  http.end();

  DynamicJsonDocument doc(12288);
  if (deserializeJson(doc, body) != DeserializationError::Ok || !doc.is<JsonArray>()) return;
  JsonArray arr = doc.as<JsonArray>();

  // Compute minute gaps (newest→older), keep up to 32 in temp buffers
  int      tmpMins[32];
  uint32_t tmpHeights[32];
  int tcount = 0;

  for (size_t i = 0; i + 1 < arr.size() && tcount < 32; ++i) {
    uint32_t t0 = arr[i]["timestamp"]   | arr[i]["time"] | 0;   // newer
    uint32_t t1 = arr[i+1]["timestamp"] | arr[i+1]["time"] | 0; // older
    if (!t0 || !t1) continue;

    long m = lround(fabs((double)t0 - (double)t1) / 60.0);
    if (m < 0)   m = 0;
    if (m > 255) m = 255;

    tmpMins[tcount]    = (int)m;
    // label each bar with the **older** block’s height (under that gap)
    tmpHeights[tcount] = (uint32_t)(arr[i+1]["height"] | 0);
    ++tcount;
  }

  // Oldest→newest into the 12 visible slots
  const int MAXN = 12;
  uint8_t  mins[MAXN];
  uint32_t heights[MAXN];
  int count = 0;

  for (int i = tcount - 1; i >= 0 && count < MAXN; --i) {
    mins[count]    = (uint8_t)tmpMins[i];
    heights[count] = tmpHeights[i];
    ++count;
  }

  if (count > 0) {
    ui_update_block_intervals(mins, count);
    ui_update_block_labels(heights, count);
  }
}







//Bitcoin Chart
void fetchBitcoinChartData(lv_chart_series_t* series, lv_obj_t* chart) {
  Serial.println(F("[chart] Fetching 24h prices…"));
  HTTPClient http;
  http.setTimeout(5000);  // 5 second timeout
  http.begin("https://api.coingecko.com/api/v3/coins/bitcoin/market_chart?vs_currency=usd&days=1");
  int httpCode = http.GET();
  Serial.printf("[chart] HTTP: %d\n", httpCode);

  if (httpCode != 200) {
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();
  
  yield(); // Feed watchdog after network operation

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[chart] JSON error: ");
    Serial.println(err.c_str());
    return;
  }
  
  yield(); // Feed watchdog after JSON parsing

  JsonArray prices = doc["prices"];
  // ADDED: grab last 24h volume (USD)
  JsonArray volsArr = doc["total_volumes"];
  float volLast = 0.0f;
  if (!volsArr.isNull() && volsArr.size() > 0) {
    volLast = volsArr[volsArr.size()-1][1] | 0.0f;
  }
  
  yield(); // Feed watchdog after array access
  
  size_t n = prices.size();
  Serial.printf("[chart] points available: %u\n", (unsigned)n);
  if (n < 2) return;

  const int targetPts = 24;   // show ~24 hourly points
  float first = 0, last = 0;
  float minv = FLT_MAX, maxv = -FLT_MAX;

  // 1) Sample ~24 points evenly across the window, compute min/max
  for (int i = 0; i < targetPts; i++) {
    size_t idx = (size_t)((float)i * (n - 1) / (targetPts - 1));
    float p = prices[idx][1]; // [timestamp, price]
    if (i == 0) first = p;
    if (i == targetPts - 1) last = p;
    if (p < minv) minv = p;
    if (p > maxv) maxv = p;

    if (chart && series) {
      // Footer chart gets the actual price values
      lv_chart_set_value_by_id(chart, series, i, (lv_coord_t)p);
    }
    
    // Feed watchdog every few iterations to prevent timeout
    if (i % 8 == 0) yield();
  }

// ADDED: update 24h stat pills
  ui_update_24h_stats(minv, maxv, volLast);
  yield(); // Feed watchdog after UI update
  

  // 2) Footer chart: apply a little padding so the line isn't touching edges
  if (chart) {
    float pad = (maxv - minv) * 0.05f;
    if (pad < 1) pad = 1; // guard tiny windows
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y,
                       (lv_coord_t)(minv - pad),
                       (lv_coord_t)(maxv + pad));
    lv_chart_refresh(chart);
    yield(); // Feed watchdog after chart operations
  }

  // 3) Mini sparkline: normalize to 0..100 and push into priceSeriesMini
  if (priceChartMini && priceSeriesMini) {
    for (int i = 0; i < targetPts; i++) {
      size_t idx = (size_t)((float)i * (n - 1) / (targetPts - 1));
      float p = prices[idx][1];
      float y = (maxv > minv) ? ( (p - minv) * 100.0f / (maxv - minv) ) : 50.0f;
      lv_chart_set_value_by_id(priceChartMini, priceSeriesMini, i, (lv_coord_t)y);
      
      // Feed watchdog every few iterations
      if (i % 8 == 0) yield();
    }
    lv_chart_refresh(priceChartMini);
    yield(); // Feed watchdog after mini chart
  }
  
  // 4) Update cache - split into smaller chunks to avoid watchdog timeout
  {
  auto& ch = Cache::chart;
  const int targetPts = 24;   // make sure this matches your DataStore array sizes

  for (int i = 0; i < targetPts; i++) {
    size_t idx = (size_t)((float)i * (n - 1) / (targetPts - 1));
    float pY = prices[idx][1];
    float y  = (maxv > minv) ? ((pY - minv) * 100.0f / (maxv - minv)) : 50.0f;

    ch.points[i] = pY;   // footer chart (USD)
    ch.mini[i]   = y;    // mini sparkline (0..100)
    
    // Feed watchdog every few iterations
    if (i % 8 == 0) yield();
  }

  ch.low  = minv;
  ch.high = maxv;
  ch.volUsd  = volLast;
  ch.valid = true;
  }
  
  Serial.println("[chart] Processing complete");
}



  
 
  
 // NEW: Toggle flags
 //bool isLeftOn = false;  
 //bool isRightOn = false;
 
 
 
 void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Spark Boot");

  //  Mount FS once (portal will serve from SPIFFS)
  SPIFFS.begin(true);

  //  Decide connection path *before* any LVGL drawing
  bool wlan_ok = connectWiFiFromPrefs();
  if (!wlan_ok) {
    startPortal();  // sets portalModeActive = true and fills g_apName
  }

// --- World/Weather init ---
load_city_tz_from_prefs();

// 📦 Load cache for instant display (prevents blank screens)
loadDisplayCache();

// 1) Start SNTP once (do NOT pass an IANA zone here)
configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

// 2) Apply timezone from portal value (IANA → POSIX mapping in world_screen.cpp)
ui_weather_set_tz_label(g_savedTZ);



// 3) Optional: choose clock style (12h for Calgary UX by default)
ui_weather_set_clock_12h(true);

// 4) Wait (up to ~10s) for RTC to be valid before first paint
time_t now = 0; uint32_t t0 = millis();
do { now = time(nullptr); if (now > 1600000000) break; delay(200); } while (millis() - t0 < 10000);

// Debug: confirm wall time and zone
{
  time_t t = time(nullptr);
  struct tm lt; localtime_r(&t, &lt);
  char buf[64];
  strftime(buf, sizeof(buf), "%c %Z (UTC%z)", &lt);
  Serial.printf("Local time check: %s  | TZ label: %s\n", buf, g_savedTZ.c_str());
}



// 5) Geocode once (if needed) and show location
String c, r, k;
if (geocode_city(g_savedCity, g_lat, g_lon, c, r, k)) {
  ui_weather_set_location(c, r, k);
}

// 6) Fetch current weather once on boot
int tC = 0; String cond;
if (fetch_weather(g_lat, g_lon, tC, cond)) {
  ui_weather_set_current(tC, cond);
}

// 7) First clock paint happens inside the world screen; this call is harmless but optional.
//    (ui_weather_set_time(...) now just calls render_time_from_rtc())
ui_weather_set_time(String());




  //  Initialize display + LVGL
  // 1. Boot LVGL + LCD
  lcd_init();

  // ---- FIRST FRAME BUG FIX FOR ST7262 ----
  // Force full-screen black to wipe corrupt right-edge/bottom rows
  lv_obj_t* wipe = lv_obj_create(NULL);
  lv_obj_set_size(wipe, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(wipe, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(wipe, LV_OPA_COVER, LV_PART_MAIN);
  lv_scr_load(wipe);
  lv_timer_handler();  // force immediate draw
  delay(60);           // allow panel to fully latch the frame
  // ----------------------------------------

  lv_ready = true;

  // 2. Initialize UI theme system
  ui::init_ui_theme();

  


    // *** STYLES ***
    lv_style_init(&orangeStyle);
    lv_style_set_bg_color(&orangeStyle, lv_color_hex(0xFFA500));
    lv_style_set_radius(&orangeStyle, 10);
    lv_style_set_bg_opa(&orangeStyle, LV_OPA_COVER);

    lv_style_init(&greenStyle);
    lv_style_set_bg_color(&greenStyle, lv_color_hex(0x00FF00));
    lv_style_set_radius(&greenStyle, 10);
    lv_style_set_bg_opa(&greenStyle, LV_OPA_COVER);

    lv_style_init(&blueStyle);
    lv_style_set_bg_color(&blueStyle, lv_color_hex(0x007BFF));
    lv_style_set_radius(&blueStyle, 10);
    lv_style_set_bg_opa(&blueStyle, LV_OPA_COVER);

    lv_style_init(&yellowStyle);
    lv_style_set_bg_color(&yellowStyle, lv_color_hex(0xFFFF00));
    lv_style_set_radius(&yellowStyle, 10);
    lv_style_set_bg_opa(&yellowStyle, LV_OPA_COVER);

    // ✨ Widget style
    lv_style_init(&widgetStyle);
    lv_style_set_radius(&widgetStyle, 16);
    lv_style_set_bg_color(&widgetStyle, lv_color_hex(0xFCA420));
    lv_style_set_bg_opa(&widgetStyle, LV_OPA_10);
    lv_style_set_border_width(&widgetStyle, 3);
    lv_style_set_border_color(&widgetStyle, lv_color_hex(0xFCA420));
    lv_style_set_border_opa(&widgetStyle, LV_OPA_60);

    // ✨ Widget2 style
    lv_style_init(&widget2Style);
    lv_style_set_radius(&widget2Style, 16);
    lv_style_set_bg_color(&widget2Style, lv_color_hex(0x777777));
    lv_style_set_bg_opa(&widget2Style, LV_OPA_20);
    lv_style_set_border_width(&widget2Style, 1);
    lv_style_set_border_color(&widget2Style, lv_color_hex(0xFCA420));
    lv_style_set_border_opa(&widget2Style, LV_OPA_60);

    // ✨ Glow effect
    lv_style_init(&glowStyle);
    lv_style_set_radius(&glowStyle, 16);
    lv_style_set_shadow_color(&glowStyle, lv_color_hex(0xFCA420));
    lv_style_set_shadow_width(&glowStyle, 15);
    lv_style_set_shadow_opa(&glowStyle, LV_OPA_70);

  //  UI based on mode: portal vs normal
  if (portalModeActive) {
    // Show the on-device instructions screen
    showportal_screen(g_apName);
  } else {
    Serial.println("🌐 Wi-Fi connected via saved preferences.");
    // Setup dashboard web server for local access
    setupDashboardServer();
    // Load dashboard only when connected
    lv_scr_load(create_metrics_screen());
    lvgl_port_unlock();
  }    Serial.println("✅ UI ready");
    delay(500);
    fetchBitcoinData();
    fetchBitcoinChartData(priceSeries, priceChart);
  }

   
   
void loop() {
  if (portalModeActive) {
    dns.processNextRequest();   // keep the captive portal DNS responsive
  } else {
    // Monitor WiFi connection when not in portal mode
    checkWiFiConnection();
  }
  
   static unsigned long lastUpdate = 0;
   if (millis() - lastUpdate > 30000) {
     fetchBitcoinData();
     yield(); // Feed watchdog after data fetch
     lastUpdate = millis();
   }static unsigned long t_hash = 0, t_blocks = 0, t_meta = 0;
unsigned long nowMs = millis();

if (nowMs - t_hash   > 30000UL) { fetch_hashrate_and_diff();   t_hash = nowMs; yield(); }
if (nowMs - t_blocks > 30000UL) { fetch_block_intervals_footer(); t_blocks = nowMs; yield(); }
if (nowMs - t_meta   > 120000UL){ fetch_market_meta();          t_meta = nowMs; yield(); }



 // Adaptive time refresh: faster (5s) until NTP is valid, then 60s
static unsigned long t1 = 0;
unsigned long period = timeIsValid() ? 60000UL : 5000UL;
if (millis() - t1 > period) {
  ui_weather_set_time(now_time_12h());
  t1 = millis();
}


// Re-render the "age" text every 60s (cheap)
static unsigned long t_age = 0;
if (millis() - t_age > 60000UL) {
  ui_tick_block_age();
  t_age = millis();
}



// Refresh weather every 30 minutes
static unsigned long t2 = 0;
if (millis() - t2 > 1800000UL) {
  int tC = 0; String cond;
  if (fetch_weather(g_lat, g_lon, tC, cond)) {
    ui_weather_set_current(tC, cond);
  } else {
    // If lat/lon unknown (or failed), try geocoding again from saved city
    String c, r, k;
    if (geocode_city(g_savedCity, g_lat, g_lon, c, r, k)) {
      ui_weather_set_location(c, r, k);
      if (fetch_weather(g_lat, g_lon, tC, cond)) {
        ui_weather_set_current(tC, cond);
      }
    }
  }
  t2 = millis();
}
}
 // --------------------------------------------------
 // 🚀 STACKSWORTH Spark – Engine Activated
 // Built with love, glow effects, and 🍊 coin self-sovereignty
