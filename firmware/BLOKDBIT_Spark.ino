/***************************************************************************************
 *  BLOKDBIT Spark – "mainScreen" UI
 *  --------------------------------------------------
 *  Project     : BLOKDBIT Spark Firmware
 *  Version     : v0.0.2
 *  Device      : ESP32-S3 Waveshare 7" Touchscreen (800x480)
 *  Description : Modular Bitcoin Dashboard UI using LVGL
 *  Designer    : Bitcoin Manor 🟧
 * 
 *  
 *  💡 Easter Egg: Try tapping the infinity label in v0.1 😉
 ***************************************************************************************/


#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Waveshare_ST7262_LVGL.h>
#include <lvgl.h>
#include "screen_manager.h"

String title = "BLOKDBIT Spark – Satscape v0.0.2";
lv_style_t widgetStyle;
lv_style_t widget2Style;
lv_style_t glowStyle;

// Wi-Fi Credentials
const char* ssid = "[ in-juh-noo-i-tee ]";
const char* password = "notachance";



int lastBlockHeight = 0;


  //trigger flash when new block arrives
void triggerBlockSurprise() {
  Serial.println("🎉 New block detected!");
 
  // Temporarily change background
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFCA420), 0);
  lv_timer_handler();  // Force LVGL to update the display immediately
  delay(150);          // Flash duration

  // Revert back
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
  lv_timer_handler();  // Again force update
}



void fetchBitcoinData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Bitcoin Price
    http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd");
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      int index = payload.indexOf(":");
      if (index != -1) {
        DynamicJsonDocument doc(512);
        deserializeJson(doc, payload);
        float price = doc["bitcoin"]["usd"];
        char priceFormatted[16];
        sprintf(priceFormatted, "$%.2f", price);
        lv_label_set_text(priceValueLabel, priceFormatted);
      }
    }
    http.end();

    // Block Height
    http.begin("https://mempool.space/api/blocks/tip/height");
    httpCode = http.GET();
    if (httpCode == 200) {
      String blockHeightStr = http.getString();
      int blockHeight = blockHeightStr.toInt();

      if (blockHeight != lastBlockHeight) {
        lastBlockHeight = blockHeight;
        triggerBlockSurprise();
      }

      lv_label_set_text(blockValueLabel, blockHeightStr.c_str());
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== " + title + " ===");
  Serial.println("Initializing display and touch...");

  lcd_init();

  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  Serial.println("Display initialized.");

  lvgl_port_lock(-1);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);


    // Set dark background
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

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

  initMainScreen();  // 🔥 Clean screen loading
  lvgl_port_unlock();

  Serial.println("Enhanced UI complete.");
  fetchBitcoinData();
}

void loop() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 30000) {
    fetchBitcoinData();
    lastUpdate = millis();
  }
  delay(1000);
}
// --------------------------------------------------
// 🚀 BLOKDBIT Spark – Engine Activated
// Built with love, glow effects, and 🍊 coin self-sovereignty
// --------------------------------------------------
