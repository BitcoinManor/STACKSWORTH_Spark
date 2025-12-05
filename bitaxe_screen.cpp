// SYACKSWORTH_Spark bitaxe_screen.cpp
// SPARKv1.0.0


#include <Arduino.h>
#include "bitaxe_screen.h"
#include "screen_manager.h"
#include "axestack_theme.h"
#include "ui_theme.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

//External shared Objects
extern lv_obj_t* backBtn;
extern lv_obj_t* rightBtn;


// External shared styles and labels 
extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;
extern lv_style_t orangeStyle;
extern lv_style_t greenStyle;
extern lv_style_t blueStyle;

// Bitaxe data storage
struct BitaxeData {
  float hashrate = 0.0f;      // GH/s
  float temp = 0.0f;          // °C
  int fanSpeed = 0;           // RPM
  float voltage = 0.0f;       // V
  float current = 0.0f;       // A
  float power = 0.0f;         // W
  int frequency = 0;          // MHz
  String bestDiff = "0";
  String poolUrl = "";
  String poolUser = "";
  int sharesAccepted = 0;
  int sharesRejected = 0;
  String uptime = "0s";
  String asicModel = "";
  String version = "";
  bool isOnline = false;
};

static BitaxeData bitaxeData;

// Label pointers for updating
static lv_obj_t* lblHashrate = nullptr;
static lv_obj_t* lblBestDiff = nullptr;
static lv_obj_t* lblPower = nullptr;
static lv_obj_t* lblTemp = nullptr;
static lv_obj_t* lblFan = nullptr;
static lv_obj_t* lblPool = nullptr;
static lv_obj_t* lblShares = nullptr;
static lv_obj_t* lblDevice = nullptr;


// Fetch data from Bitaxe running AxeOS (default IP: 10.0.0.200)
void fetchBitaxeData(const char* ip = "10.0.0.200") {
  HTTPClient http;
  String url = String("http://") + ip + "/api/system/info";
  
  http.setTimeout(3000);
  if (!http.begin(url)) {
    Serial.println("❌ Bitaxe: Failed to begin HTTP");
    bitaxeData.isOnline = false;
    return;
  }
  
  int code = http.GET();
  if (code != 200) {
    Serial.printf("❌ Bitaxe: HTTP %d\n", code);
    http.end();
    bitaxeData.isOnline = false;
    return;
  }
  
  String payload = http.getString();
  http.end();
  
  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, payload)) {
    Serial.println("❌ Bitaxe: JSON parse failed");
    bitaxeData.isOnline = false;
    return;
  }
  
  // Parse AxeOS API response (exact field names from AxeOS)
  bitaxeData.hashrate = doc["hashRate"] | 0.0f;  // GH/s
  bitaxeData.temp = doc["temp"] | 0.0f;  // °C
  bitaxeData.fanSpeed = doc["fanSpeed"] | 0;  // RPM
  bitaxeData.voltage = doc["voltage"] | 0.0f;  // V
  bitaxeData.current = doc["current"] | 0.0f;  // A
  bitaxeData.power = doc["power"] | 0.0f;  // W
  bitaxeData.frequency = doc["frequency"] | 0;  // MHz
  bitaxeData.bestDiff = String(doc["bestDiff"].as<const char*>());  // Best difficulty string
  bitaxeData.sharesAccepted = doc["sharesAccepted"] | 0;
  bitaxeData.sharesRejected = doc["sharesRejected"] | 0;
  bitaxeData.uptime = String(doc["uptimeSeconds"].as<int>());
  bitaxeData.asicModel = doc["ASICModel"].as<String>();  // "BM1397", "BM1366", etc.
  bitaxeData.version = doc["version"].as<String>();  // AxeOS version
  bitaxeData.poolUrl = doc["stratumURL"].as<String>();  // Pool URL from device
  bitaxeData.poolUser = doc["stratumUser"].as<String>();  // Pool user from device
  bitaxeData.isOnline = true;
  
  Serial.printf("✅ Bitaxe: %.2f GH/s @ %.1f°C [%s]\n", 
                bitaxeData.hashrate, bitaxeData.temp, bitaxeData.asicModel.c_str());
}

// Fetch additional pool stats from Bitcoin Manor solo pool (optional)
void fetchPoolInfo() {
  // Note: Pool info already comes from AxeOS /api/system/info
  // This function can fetch additional pool-side stats if needed
  
  HTTPClient http;
  String url = "http://solo.bitcoinmanor.com:3334/api/info";
  
  http.setTimeout(3000);
  if (!http.begin(url)) {
    Serial.println("⚠️ Pool API: Failed to connect (using AxeOS pool info)");
    return;
  }
  
  int code = http.GET();
  if (code != 200) {
    Serial.printf("⚠️ Pool API: HTTP %d (using AxeOS pool info)\n", code);
    http.end();
    return;
  }
  
  String payload = http.getString();
  http.end();
  
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, payload)) {
    Serial.println("⚠️ Pool API: JSON parse failed (using AxeOS pool info)");
    return;
  }
  
  // Optional: Extract additional pool-side statistics here
  // For now, we rely on AxeOS device data which is more reliable
  
  Serial.println("✅ Pool API stats available");
}

// Update all labels with current data
void updateBitaxeLabels() {
  if (!bitaxeData.isOnline) {
    if (lblHashrate) lv_label_set_text(lblHashrate, "OFFLINE");
    return;
  }
  
  char buf[64];
  
  // W1: Hashrate
  if (lblHashrate) {
    snprintf(buf, sizeof(buf), "%.1f\nGH/s", bitaxeData.hashrate);
    lv_label_set_text(lblHashrate, buf);
  }
  
  // W2: Best Difficulty
  if (lblBestDiff) {
    lv_label_set_text(lblBestDiff, bitaxeData.bestDiff.c_str());
  }
  
  // W3: Power
  if (lblPower) {
    snprintf(buf, sizeof(buf), "%.1fV\n%.2fA\n%.1fW", 
             bitaxeData.voltage, bitaxeData.current, bitaxeData.power);
    lv_label_set_text(lblPower, buf);
  }
  
  // W4: Temperature
  if (lblTemp) {
    snprintf(buf, sizeof(buf), "%.0f°C", bitaxeData.temp);
    lv_label_set_text(lblTemp, buf);
  }
  
  // W5: Fan
  if (lblFan) {
    snprintf(buf, sizeof(buf), "%d\nRPM", bitaxeData.fanSpeed);
    lv_label_set_text(lblFan, buf);
  }
  
  // W6: Pool
  if (lblPool) {
    snprintf(buf, sizeof(buf), "%s\n%s", 
             bitaxeData.poolUrl.c_str(), 
             bitaxeData.poolUser.substring(0, 20).c_str());
    lv_label_set_text(lblPool, buf);
  }
  
  // W7: Shares
  if (lblShares) {
    float reject_rate = (bitaxeData.sharesAccepted + bitaxeData.sharesRejected) > 0 
      ? (100.0f * bitaxeData.sharesRejected / (bitaxeData.sharesAccepted + bitaxeData.sharesRejected))
      : 0.0f;
    snprintf(buf, sizeof(buf), "✓ %d  ✗ %d\n%.1f%% reject", 
             bitaxeData.sharesAccepted, bitaxeData.sharesRejected, reject_rate);
    lv_label_set_text(lblShares, buf);
  }
  
  // W8: Device
  if (lblDevice) {
    snprintf(buf, sizeof(buf), "%s\n%d MHz\nv%s", 
             bitaxeData.asicModel.c_str(), bitaxeData.frequency, bitaxeData.version.c_str());
    lv_label_set_text(lblDevice, buf);
  }
}

// Public update function called from main loop
void ui_update_bitaxe_screen(const char* bitaxe_ip = "10.0.0.200") {
  fetchBitaxeData(bitaxe_ip);  // Fetch from AxeOS device
  fetchPoolInfo();  // Optional: fetch pool-side stats
  updateBitaxeLabels();
}

// Handle button press events
void onTouchEvent_bitaxe_screen(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == backBtn) {
    Serial.println("➡️ Back button pressed: Switching to World Screen");
    load_screen(2);
  } else if (target == rightBtn) {
    Serial.println("➡️ Right button pressed: Switching to Settings Screen");
    load_screen(4);
  }
}


// Make a dark card with a thin orange outline (no white bg, slight glow)
static lv_obj_t* make_orange_card(lv_obj_t* parent, const char* title) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_remove_style_all(card);                         // start clean

  // size: half width (2 columns), auto height
  lv_obj_set_size(card, LV_PCT(49), LV_SIZE_CONTENT);

  // dark body
  lv_obj_set_style_bg_color(card, lv_color_hex(0x14181D), 0);
  lv_obj_set_style_bg_opa  (card, LV_OPA_COVER,          0);

  // thin orange border + soft glow
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_shadow_color(card, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_shadow_opa  (card, LV_OPA_20, 0);     // subtle halo
  lv_obj_set_style_shadow_width(card, 8, 0);
  lv_obj_set_style_shadow_spread(card, 0, 0);
  lv_obj_set_style_shadow_ofs_x(card, 0, 0);
  lv_obj_set_style_shadow_ofs_y(card, 0, 0);

  // compact inner spacing and vertical stack
  lv_obj_set_style_pad_all(card, 8, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(card, 4, 0);

  // title
  lv_obj_t* titleLbl = lv_label_create(card);
  lv_label_set_text(titleLbl, title);
  lv_obj_set_style_text_color(titleLbl, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(titleLbl, &lv_font_montserrat_14, 0);

  return card;
}



lv_obj_t* create_bitaxe_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);

  axe_apply_bg(scr);  // uses tokens.bg

  // Header: STACKSWORTH // SPARK (tokenized colors)
  lv_obj_t* lblStacksworth = lv_label_create(scr);
  lv_label_set_text(lblStacksworth, "STACKSWORTH");
  lv_obj_set_style_text_color(lblStacksworth, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(lblStacksworth, &lv_font_montserrat_20, 0);
  lv_obj_align(lblStacksworth, LV_ALIGN_TOP_LEFT, axe::spacing::lg, axe::spacing::sm + 2);

  lv_obj_t* lblSpark = lv_label_create(scr);
  lv_label_set_text(lblSpark, "// SPARK");
  lv_obj_set_style_text_color(lblSpark, lv_color_hex(axe::color::text), 0);
  lv_obj_set_style_text_font(lblSpark, &lv_font_montserrat_20, 0);
  lv_obj_align(lblSpark, LV_ALIGN_TOP_LEFT, axe::spacing::md + 175, axe::spacing::sm + 2);

  // Top-right title: AxeStack \\ Bitaxe Hub
  lv_obj_t* lblBitaxe = lv_label_create(scr);
  lv_label_set_text(lblBitaxe, " \\\\ Bitaxe Hub");   // shows as "\\ Bitaxe Hub"
  lv_obj_set_style_text_color(lblBitaxe, lv_color_hex(axe::color::text), 0);
  lv_obj_set_style_text_font(lblBitaxe, &lv_font_montserrat_20, 0);
  lv_obj_align(lblBitaxe, LV_ALIGN_TOP_RIGHT, -axe::spacing::xl, axe::spacing::sm + 2);

  lv_obj_t* lblAxeStack = lv_label_create(scr);
  lv_label_set_text(lblAxeStack, "AxeStack");
  lv_obj_set_style_text_color(lblAxeStack, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(lblAxeStack, &lv_font_montserrat_20, 0);
  lv_obj_align_to(lblAxeStack, lblBitaxe, LV_ALIGN_OUT_LEFT_MID, -axe::spacing::xs - 2, 0);

  // ============================================================
  //   BITAXE WIDGET CLUSTER (190mm x 140mm → scaled panel)
  //   8 empty cards laid out exactly like your blueprint
  // ============================================================

  // Wider layout for better 7" screen utilization, keeping height manageable
  // due to title/version space constraints at top and bottom.
  const int FRAME_INNER_W = 700;   // wider for better screen usage  
  const int FRAME_INNER_H = 400;   // keep original height (title/version limits)
  const int FRAME_PAD     = 12;    // inner margin between border and widgets

  const int FRAME_W = FRAME_INNER_W + FRAME_PAD * 2;
  const int FRAME_H = FRAME_INNER_H + FRAME_PAD * 2;

  // Outer framed container with soft neon cyan glow
  lv_obj_t* frame = lv_obj_create(scr);
  lv_obj_remove_style_all(frame);
  lv_obj_set_size(frame, FRAME_W, FRAME_H);

  // Dark background for the panel
  lv_obj_set_style_bg_color(frame, lv_color_hex(0x05070A), 0);
  lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);

  // Neon cyan border + soft glow
  lv_obj_set_style_border_width(frame, 2, 0);
  lv_obj_set_style_border_color(frame, lv_color_hex(0x00F0FF), 0);
  lv_obj_set_style_radius(frame, 12, 0);

  lv_obj_set_style_shadow_color(frame, lv_color_hex(0x00F0FF), 0);
  lv_obj_set_style_shadow_opa(frame, LV_OPA_30, 0);
  lv_obj_set_style_shadow_width(frame, 18, 0);
  lv_obj_set_style_shadow_spread(frame, 0, 0);
  lv_obj_set_style_shadow_ofs_x(frame, 0, 0);
  lv_obj_set_style_shadow_ofs_y(frame, 0, 0);

  // Position the panel towards the bottom, leaving room for headers and footer
  lv_obj_align(frame, LV_ALIGN_BOTTOM_MID, 0, -axe::spacing::lg);

  // Helper to create an empty widget card (no labels/icons)
  auto make_empty_widget = [](lv_obj_t* parent) -> lv_obj_t* {
    lv_obj_t* card = ui::make_card(parent);  // keeps global card style
    // No labels or content – pure empty widget
    return card;
  };

  // NOTE: All positions below are scaled from your mm layout
  // using a factor of 400px / 140mm. These ints are precomputed.

  // ===== Widget 1: HASHRATE =====
  lv_obj_t* w1 = make_empty_widget(frame);
  lv_obj_set_size(w1, 147, 114);
  lv_obj_set_pos (w1, FRAME_PAD + 0, FRAME_PAD + 0);
  
  lv_obj_t* w1Title = lv_label_create(w1);
  lv_label_set_text(w1Title, "HASHRATE");
  lv_obj_set_style_text_color(w1Title, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(w1Title, &lv_font_montserrat_12, 0);
  lv_obj_align(w1Title, LV_ALIGN_TOP_LEFT, 5, 5);
  
  lblHashrate = lv_label_create(w1);
  lv_label_set_text(lblHashrate, "0.0\nGH/s");
  lv_obj_set_style_text_color(lblHashrate, lv_color_hex(0x00FF88), 0);
  lv_obj_set_style_text_font(lblHashrate, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_align(lblHashrate, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lblHashrate, LV_ALIGN_CENTER, 0, 5);

  // ===== Widget 2: BEST DIFFICULTY =====
  lv_obj_t* w2 = make_empty_widget(frame);
  lv_obj_set_size(w2, 313, 114);
  lv_obj_set_pos (w2, FRAME_PAD + 129, FRAME_PAD + 0);
  
  lv_obj_t* w2Title = lv_label_create(w2);
  lv_label_set_text(w2Title, "BEST DIFFICULTY");
  lv_obj_set_style_text_color(w2Title, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(w2Title, &lv_font_montserrat_12, 0);
  lv_obj_align(w2Title, LV_ALIGN_TOP_LEFT, 5, 5);
  
  lblBestDiff = lv_label_create(w2);
  lv_label_set_text(lblBestDiff, "0");
  lv_obj_set_style_text_color(lblBestDiff, lv_color_hex(0xFFD700), 0);
  lv_obj_set_style_text_font(lblBestDiff, &lv_font_montserrat_28, 0);
  lv_obj_align(lblBestDiff, LV_ALIGN_CENTER, 0, 5);

  // ===== Widget 3: POWER STATS =====
  lv_obj_t* w3 = make_empty_widget(frame);
  lv_obj_set_size(w3, 202, 214);
  lv_obj_set_pos (w3, FRAME_PAD + 386, FRAME_PAD + 0);
  
  lv_obj_t* w3Title = lv_label_create(w3);
  lv_label_set_text(w3Title, "POWER");
  lv_obj_set_style_text_color(w3Title, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(w3Title, &lv_font_montserrat_12, 0);
  lv_obj_align(w3Title, LV_ALIGN_TOP_LEFT, 5, 5);
  
  lblPower = lv_label_create(w3);
  lv_label_set_text(lblPower, "0.0V\n0.00A\n0.0W");
  lv_obj_set_style_text_color(lblPower, lv_color_hex(0x00CCFF), 0);
  lv_obj_set_style_text_font(lblPower, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_align(lblPower, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lblPower, LV_ALIGN_CENTER, 0, 10);

  // ===== Widget 4: TEMPERATURE =====
  lv_obj_t* w4 = make_empty_widget(frame);
  lv_obj_set_size(w4, 147, 114);
  lv_obj_set_pos (w4, FRAME_PAD + 0, FRAME_PAD + 129);
  
  lv_obj_t* w4Title = lv_label_create(w4);
  lv_label_set_text(w4Title, "TEMP");
  lv_obj_set_style_text_color(w4Title, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(w4Title, &lv_font_montserrat_12, 0);
  lv_obj_align(w4Title, LV_ALIGN_TOP_LEFT, 5, 5);
  
  lblTemp = lv_label_create(w4);
  lv_label_set_text(lblTemp, "0°C");
  lv_obj_set_style_text_color(lblTemp, lv_color_hex(0xFF6B6B), 0);
  lv_obj_set_style_text_font(lblTemp, &lv_font_montserrat_24, 0);
  lv_obj_align(lblTemp, LV_ALIGN_CENTER, 0, 5);

  // ===== Widget 5: FAN SPEED =====
  lv_obj_t* w5 = make_empty_widget(frame);
  lv_obj_set_size(w5, 147, 114);
  lv_obj_set_pos (w5, FRAME_PAD + 129, FRAME_PAD + 129);
  
  lv_obj_t* w5Title = lv_label_create(w5);
  lv_label_set_text(w5Title, "FAN");
  lv_obj_set_style_text_color(w5Title, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(w5Title, &lv_font_montserrat_12, 0);
  lv_obj_align(w5Title, LV_ALIGN_TOP_LEFT, 5, 5);
  
  lblFan = lv_label_create(w5);
  lv_label_set_text(lblFan, "0\nRPM");
  lv_obj_set_style_text_color(lblFan, lv_color_hex(0x88DDFF), 0);
  lv_obj_set_style_text_font(lblFan, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_align(lblFan, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lblFan, LV_ALIGN_CENTER, 0, 5);

  // ===== Widget 6: POOL INFO =====
  lv_obj_t* w6 = make_empty_widget(frame);
  lv_obj_set_size(w6, 147, 271);
  lv_obj_set_pos (w6, FRAME_PAD + 257, FRAME_PAD + 129);
  
  lv_obj_t* w6Title = lv_label_create(w6);
  lv_label_set_text(w6Title, "POOL");
  lv_obj_set_style_text_color(w6Title, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(w6Title, &lv_font_montserrat_12, 0);
  lv_obj_align(w6Title, LV_ALIGN_TOP_LEFT, 5, 5);
  
  lblPool = lv_label_create(w6);
  lv_label_set_text(lblPool, "solo.bitcoinmanor\n.com:3334");
  lv_obj_set_style_text_color(lblPool, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(lblPool, &lv_font_montserrat_14, 0);
  lv_label_set_long_mode(lblPool, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(lblPool, 130);
  lv_obj_align(lblPool, LV_ALIGN_CENTER, 0, 10);

  // ===== Widget 7: SESSION STATS =====
  lv_obj_t* w7 = make_empty_widget(frame);
  lv_obj_set_size(w7, 313, 143);
  lv_obj_set_pos (w7, FRAME_PAD + 0, FRAME_PAD + 257);
  
  lv_obj_t* w7Title = lv_label_create(w7);
  lv_label_set_text(w7Title, "SHARES");
  lv_obj_set_style_text_color(w7Title, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(w7Title, &lv_font_montserrat_12, 0);
  lv_obj_align(w7Title, LV_ALIGN_TOP_LEFT, 5, 5);
  
  lblShares = lv_label_create(w7);
  lv_label_set_text(lblShares, "✓ 0  ✗ 0\n0.0% reject");
  lv_obj_set_style_text_color(lblShares, lv_color_hex(0xCCCCCC), 0);
  lv_obj_set_style_text_font(lblShares, &lv_font_montserrat_18, 0);
  lv_obj_align(lblShares, LV_ALIGN_CENTER, 0, 10);

  // ===== Widget 8: DEVICE INFO =====
  lv_obj_t* w8 = make_empty_widget(frame);
  lv_obj_set_size(w8, 202, 171);
  lv_obj_set_pos (w8, FRAME_PAD + 386, FRAME_PAD + 229);
  
  lv_obj_t* w8Title = lv_label_create(w8);
  lv_label_set_text(w8Title, "DEVICE");
  lv_obj_set_style_text_color(w8Title, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(w8Title, &lv_font_montserrat_12, 0);
  lv_obj_align(w8Title, LV_ALIGN_TOP_LEFT, 5, 5);
  
  lblDevice = lv_label_create(w8);
  lv_label_set_text(lblDevice, "BitAxe\n0 MHz\nv0.0.0");
  lv_obj_set_style_text_color(lblDevice, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(lblDevice, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_align(lblDevice, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lblDevice, LV_ALIGN_CENTER, 0, 10);

  // ============================================================
  // Footer info (keeps your text, switches to tokens)
  // ============================================================
  lv_obj_t* inf = axe_make_label(scr, "SPARK v0.0.3", axe::type::body, axe::color::text);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -(axe::spacing::lg + 36), -axe::spacing::xs);

  // ============================================================
  // Navigation BUTTONS
  // ============================================================

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_BOTTOM_LEFT, 25, -60);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_bitaxe_screen, LV_EVENT_CLICKED, NULL);

  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_BOTTOM_LEFT, 70, -60);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_bitaxe_screen, LV_EVENT_CLICKED, NULL);

  return scr;
}
