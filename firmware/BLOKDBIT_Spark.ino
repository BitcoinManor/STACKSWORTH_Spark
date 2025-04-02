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
#include <Waveshare_ST7262_LVGL.h>
#include <lvgl.h>

String title = "BLOKDBIT Spark – Satscape v0.0.2";
lv_style_t widgetStyle;
lv_style_t widget2Style;
lv_style_t glowStyle;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== " + title + " ===");
  Serial.println("Initializing display and touch...");

  lcd_init();

  Serial.println("Display initialized.");
  Serial.println("Creating enhanced futuristic UI...");

  lvgl_port_lock(-1);

  // Set dark background
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

  // 🧩 Widget style
  lv_style_init(&widgetStyle);
  lv_style_set_radius(&widgetStyle, 16);
  lv_style_set_bg_color(&widgetStyle, lv_color_hex(0xFCA420));
  lv_style_set_bg_opa(&widgetStyle, LV_OPA_10);
  lv_style_set_border_width(&widgetStyle, 3);
  lv_style_set_border_color(&widgetStyle, lv_color_hex(0xFCA420));
  lv_style_set_border_opa(&widgetStyle, LV_OPA_60);


  // 🧩 Widget2 style
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

  // 🔲 Main Panel Container (95% of screen with 20px padding)
  lv_obj_t* mainPanel = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(mainPanel);
  lv_obj_set_size(mainPanel, 760, 440);
  lv_obj_align(mainPanel, LV_ALIGN_TOP_LEFT, 20, 20);
  lv_obj_set_style_bg_opa(mainPanel, LV_OPA_TRANSP, 0);
  lv_obj_add_style(mainPanel, &widgetStyle, 0);
  lv_obj_add_style(mainPanel, &glowStyle, 0);

  
  // 🟠 BLOKDBIT Label (inside mainPanel)
  lv_obj_t* blokdbitLabel = lv_label_create(mainPanel);
  lv_label_set_text(blokdbitLabel, "BLOKDBIT");
  lv_obj_set_style_text_color(blokdbitLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(blokdbitLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(blokdbitLabel, LV_ALIGN_TOP_RIGHT, -80, 20);

  // 🟠 SPARK Label (inside mainPanel)
  lv_obj_t* sparkLabel = lv_label_create(mainPanel);
  lv_label_set_text(sparkLabel, "SPARK");
  lv_obj_set_style_text_color(sparkLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(sparkLabel, &lv_font_montserrat_14, 0);
  lv_obj_align(sparkLabel, LV_ALIGN_TOP_RIGHT, -80, 40);
  



  // 🧩 Widget 1 – Top Left (inside mainPanel)
  lv_obj_t* widget1 = lv_obj_create(mainPanel);
  lv_obj_set_size(widget1, 325, 200);
  lv_obj_align(widget1, LV_ALIGN_TOP_LEFT, 40, 20);
  lv_obj_add_style(widget1, &widget2Style, 0);
  lv_obj_add_style(widget1, &glowStyle, 0);

  // 🟠Price Label (inside widget1)
  lv_obj_t* priceLabel = lv_label_create(widget1);
  lv_label_set_text(priceLabel, "BITCOIN PRICE");
  lv_obj_set_style_text_color(priceLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(priceLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(priceLabel, LV_ALIGN_TOP_MID, 0, 0);

  // 🧩 Widget 2 – Top Right (inside mainPanel)
  lv_obj_t* widget2 = lv_obj_create(mainPanel);
  lv_obj_set_size(widget2, 325, 160);
  lv_obj_align(widget2, LV_ALIGN_TOP_RIGHT, -40, 85);
  lv_obj_add_style(widget2, &widget2Style, 0);
  lv_obj_add_style(widget2, &glowStyle, 0);

  // 🟠Block Height Label (inside widget2)
  lv_obj_t* blockLabel = lv_label_create(widget2);
  lv_label_set_text(blockLabel, "BLOCK HEIGHT");
  lv_obj_set_style_text_color(blockLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(blockLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(blockLabel, LV_ALIGN_TOP_RIGHT, 0, 0);

  // 🔲  Footer Panel (inside mainPanel)
  lv_obj_t* footer = lv_obj_create(mainPanel);
  lv_obj_set_size(footer, 680, 160);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_add_style(footer, &widget2Style, 0);
  lv_obj_add_style(footer, &glowStyle, 0);

  // 🟠FEE RATES Label (inside footer)
  lv_obj_t* feeLabel = lv_label_create(footer);
  lv_label_set_text(feeLabel, "FEE RATES");
  lv_obj_set_style_text_color(feeLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(feeLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(feeLabel, LV_ALIGN_TOP_LEFT, 0, 0);

  

  // 🔁 SPARK Version Label (inside footer)
  lv_obj_t* inf = lv_label_create(footer);
  lv_label_set_text(inf, "SPARK v0.01");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  lvgl_port_unlock();
  Serial.println("Enhanced UI complete.");
}

void loop() {
  delay(1000);
}

// --------------------------------------------------
// 🚀 BLOKDBIT Spark – Engine Activated
// Built with love, glow effects, and 🍊 coin self-sovereignty
// --------------------------------------------------
