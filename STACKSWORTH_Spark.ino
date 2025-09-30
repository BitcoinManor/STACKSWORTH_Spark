/***************************************************************************************
 *  STACKSWORTH Spark – "mainScreen" UI
 *  --------------------------------------------------
 *  Project     : STACKSWORTH Spark Firmware
 *  Version     : v0.0.4
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




// ==== Captive portal globals & helpers ====
AsyncWebServer server(80);
DNSServer dns;
Preferences prefs;

IPAddress apIP(192,168,4,1);
bool portalModeActive = false;
bool lv_ready = false;        // set true after lcd_init/LVGL init
String g_apName;              // <-- needed by startPortal + LVGL pre-screen


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


// Try connecting from saved preferences
bool connectWiFiFromPrefs(uint32_t timeoutMs=15000) {
  prefs.begin("sw", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.isEmpty()) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("🌐 Connecting to %s", ssid.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250); Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("✅ WiFi OK: %s\n", WiFi.localIP().toString().c_str());
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

  // -------- Serve the portal file (handles .gz correctly) --------
server.on("/", HTTP_GET, [](AsyncWebServerRequest* r){
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
});

// Friendly aliases (optional)
server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* r){ r->redirect("/"); });
server.on("/portal",     HTTP_GET, [](AsyncWebServerRequest* r){ r->redirect("/"); });

// Captive convenience: redirect any stray path to root
server.onNotFound([](AsyncWebServerRequest* r){ r->redirect("/"); });

// Probe endpoint to verify the server is up
server.on("/ping", HTTP_GET, [](AsyncWebServerRequest* r){ r->send(200, "text/plain", "pong"); });


  // urlencoded form support (existing Matrix portal posts will hit this)
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *req){
    if (req->hasParam("ssid", true)) {
      prefs.begin("sw", false);
      prefs.putString("ssid", req->arg("ssid"));
      prefs.putString("pass", req->arg("password"));
      prefs.putString("city", req->arg("city"));
      prefs.putString("tz",   req->arg("timezone"));
      prefs.putString("device", req->arg("device"));
      prefs.end();
      Serial.println("💾 Saved portal fields (urlencoded).");
    }
    req->send(200, "text/plain", "OK");
  });

  // JSON body support
server.onRequestBody([](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t){
  if (req->url() != "/save") return;

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

  if (ssid.length()) {
    prefs.begin("sw", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.putString("city", city);
    prefs.putString("tz",   tz);
    prefs.putString("device", device);
    prefs.end();
    Serial.println("💾 Saved portal fields (JSON).");
  }
});

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Rebooting…");
    delay(250);
    ESP.restart();
  });

  server.begin();

// 👉 Show on-screen instructions
  //showPortalScreen(apName);
g_apName = apName;  // remember until LVGL ready



  Serial.printf("📶 AP: %s — connect and open http://192.168.4.1\n", apName.c_str());
}


// ===== World/Weather globals =====
static String g_savedCity;
static String g_savedTZ;     // e.g. "America/Edmonton"
static float  g_lat = 0.0f;
static float  g_lon = 0.0f;


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
  prefs.end();
  Serial.printf("🌆 City: %s  |  TZ: %s\n", g_savedCity.c_str(), g_savedTZ.c_str());
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

  WiFiClientSecure client;
  client.setInsecure();  // baby-step

  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
               "&longitude=" + String(lon, 6) +
               "&current=temperature_2m,weather_code&timezone=auto";
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return false;

  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();
  if (!payload.length()) return false;

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, payload)) return false;

  float t = doc["current"]["temperature_2m"] | NAN;
  int   w = doc["current"]["weather_code"]   | -1;
  if (isnan(t) || w < 0) return false;

  tempC = (int)t;
  condition = mapWeatherCode(w);
  Serial.printf("⛅ Weather: %d°C, %s (code %d)\n", tempC, condition.c_str(), w);
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


 

 // 🌐 Cached values for display labels (prevent flicker / uploading state)
 String lastPrice = "…";            // Show loading dots initially
 String lastFee = "…";
 String lastBlockHeight = "…";
 String lastMiner = "…";



void showPortalScreen(const String& apName) {
  if (!lv_ready) return;
  if (portalScreen) { lv_scr_load(portalScreen); return; }

  portalScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(portalScreen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(portalScreen, LV_OPA_COVER, 0);

  // Title
  lv_obj_t* title = lv_label_create(portalScreen);
  lv_label_set_text(title, "Connect to configure Wi-Fi");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);

  // Spinner (nice “working” hint)
  lv_obj_t* spin = lv_spinner_create(portalScreen, 1000, 90);
  lv_obj_set_size(spin, 48, 48);
  lv_obj_align(spin, LV_ALIGN_TOP_MID, 0, 64);

  // AP name
  portalNetLabel = lv_label_create(portalScreen);
  lv_label_set_text_fmt(portalNetLabel, "Network: %s", apName.c_str());
  lv_obj_set_style_text_color(portalNetLabel, lv_color_white(), 0);
  lv_obj_set_style_text_align(portalNetLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(portalNetLabel, LV_ALIGN_CENTER, 0, -6);

  // URL (AP IP is 192.168.4.1 by default)
  portalUrlLabel = lv_label_create(portalScreen);
  lv_label_set_text(portalUrlLabel, "If portal doesn’t auto-open:\nhttp://192.168.4.1");
  lv_obj_set_style_text_color(portalUrlLabel, lv_color_white(), 0);
  lv_obj_set_style_text_align(portalUrlLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(portalUrlLabel, LV_ALIGN_CENTER, 0, 28);

  // Small hint at bottom
  lv_obj_t* hint = lv_label_create(portalScreen);
  lv_label_set_text(hint, "Open Wi-Fi settings → join the network above.");
  lv_obj_set_style_text_color(hint, lv_color_white(), 0);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -18);

  lv_scr_load(portalScreen);
}



 
 void fetchBitcoinData() {
  if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // Step 1: Fetch Bitcoin Price (USD + CAD)
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





     // Step 2: Fetch Block Height
http.begin("https://mempool.space/api/blocks/tip/height");
httpCode = http.GET();
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



     // Step 3: Fetch Fee Estimates
      http.begin("https://mempool.space/api/v1/fees/recommended");
      httpCode = http.GET();
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



      // Step 4: Fetch Miner Information
      // Get the block hash
      String blockHash = "";
     
      http.begin("https://mempool.space/api/blocks/tip/hash");
      httpCode = http.GET();
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

  // Build minute gaps between consecutive blocks
  const int MAXN = 12;
  uint8_t mins[MAXN];  // UI expects uint8_t
  int count = 0;

  // Collect diffs (API is newest→older); store in a small temp buffer
  int tmp[32];
  int tcount = 0;
  for (size_t i = 0; i + 1 < arr.size() && tcount < 32; ++i) {
    uint32_t t0 = arr[i]["timestamp"] | arr[i]["time"] | 0;
    uint32_t t1 = arr[i+1]["timestamp"] | arr[i+1]["time"] | 0;
    if (!t0 || !t1) continue;

    long m = lround(fabs((double)t0 - (double)t1) / 60.0);
    if (m < 0)   m = 0;
    if (m > 255) m = 255;     // clamp for uint8_t

    tmp[tcount++] = (int)m;
  }

  // Take last 12, reversed to oldest→newest (what your UI wants)
  for (int i = tcount - 1; i >= 0 && count < MAXN; --i) {
    mins[count++] = (uint8_t)tmp[i];
  }

  if (count > 0) {
    ui_update_block_intervals(mins, count);
  }
}







//Bitcoin Chart
void fetchBitcoinChartData(lv_chart_series_t* series, lv_obj_t* chart) {
  Serial.println(F("[chart] Fetching 24h prices…"));
  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/coins/bitcoin/market_chart?vs_currency=usd&days=1");
  int httpCode = http.GET();
  Serial.printf("[chart] HTTP: %d\n", httpCode);

  if (httpCode != 200) {
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[chart] JSON error: ");
    Serial.println(err.c_str());
    return;
  }

  JsonArray prices = doc["prices"];
  // ADDED: grab last 24h volume (USD)
  JsonArray volsArr = doc["total_volumes"];
  float volLast = 0.0f;
  if (!volsArr.isNull() && volsArr.size() > 0) {
    volLast = volsArr[volsArr.size()-1][1] | 0.0f;
  }
  
  
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
  }

// ADDED: update 24h stat pills
  ui_update_24h_stats(minv, maxv, volLast);
  

  // 2) Footer chart: apply a little padding so the line isn't touching edges
  if (chart) {
    float pad = (maxv - minv) * 0.05f;
    if (pad < 1) pad = 1; // guard tiny windows
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y,
                       (lv_coord_t)(minv - pad),
                       (lv_coord_t)(maxv + pad));
    lv_chart_refresh(chart);
  }

  // 3) Mini sparkline: normalize to 0..100 and push into priceSeriesMini
  if (priceChartMini && priceSeriesMini) {
    for (int i = 0; i < targetPts; i++) {
      size_t idx = (size_t)((float)i * (n - 1) / (targetPts - 1));
      float p = prices[idx][1];
      float y = (maxv > minv) ? ( (p - minv) * 100.0f / (maxv - minv) ) : 50.0f;
      lv_chart_set_value_by_id(priceChartMini, priceSeriesMini, i, (lv_coord_t)y);
    }
    lv_chart_refresh(priceChartMini);
  }
  {
  auto& ch = Cache::chart;
  const int targetPts = 24;   // make sure this matches your DataStore array sizes

  for (int i = 0; i < targetPts; i++) {
    size_t idx = (size_t)((float)i * (n - 1) / (targetPts - 1));
    float pY = prices[idx][1];
    float y  = (maxv > minv) ? ((pY - minv) * 100.0f / (maxv - minv)) : 50.0f;

    ch.points[i] = pY;   // footer chart (USD)
    ch.mini[i]   = y;    // mini sparkline (0..100)
  }

  ch.low  = minv;
  ch.high = maxv;
  ch.volUsd  = volLast;
  ch.valid = true;
}
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
  lcd_init();
  lv_ready = true;

  //  UI based on mode: portal vs normal
  if (portalModeActive) {
    // Show the on-device instructions screen
    showPortalScreen(g_apName);
    // (Do NOT load metrics screen here; keep device in setup mode)
  } else {
    Serial.println("🌐 Wi-Fi connected via saved preferences.");

    // Your existing UI init path
    lvgl_port_lock(-1);
    ui::init_ui_theme();         // ← initialize theme once

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

    // Load dashboard only when connected
    lv_scr_load(create_metrics_screen());
    lvgl_port_unlock();

    Serial.println("✅ UI ready");
    delay(500);
    fetchBitcoinData();
    fetchBitcoinChartData(priceSeries, priceChart);
  }
}

   
   
   void loop() {
     if (portalModeActive) {
    dns.processNextRequest();   // keep the captive portal DNS responsive
  }
   static unsigned long lastUpdate = 0;
   if (millis() - lastUpdate > 30000) {
     fetchBitcoinData();
     lastUpdate = millis();
   }
   //delay(1000);

static unsigned long t_hash = 0, t_blocks = 0, t_meta = 0;
unsigned long nowMs = millis();

if (nowMs - t_hash   > 30000UL) { fetch_hashrate_and_diff();   t_hash = nowMs; }
if (nowMs - t_blocks > 30000UL) { fetch_block_intervals_footer(); t_blocks = nowMs; }
if (nowMs - t_meta   > 120000UL){ fetch_market_meta();          t_meta = nowMs; }



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
