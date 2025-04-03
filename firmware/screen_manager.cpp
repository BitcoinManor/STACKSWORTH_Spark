//BLOKDBIT_Spark screen_manager.cpp
//SPARKv0.02

#include <lvgl.h>
#include "screen_manager.h"

// Label definitions (global, used across files)
lv_obj_t* priceValueLabel = nullptr;
lv_obj_t* blockValueLabel = nullptr;

extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;

void initMainScreen() {
  // 🔲 Main Panel Container (95% of screen with 20px padding)
  lv_obj_t* mainPanel = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(mainPanel);
  lv_obj_set_size(mainPanel, 760, 440);
  lv_obj_align(mainPanel, LV_ALIGN_TOP_LEFT, 20, 20);
  lv_obj_set_style_bg_opa(mainPanel, LV_OPA_TRANSP, 0);
  lv_obj_add_style(mainPanel, &widgetStyle, 0);
  lv_obj_add_style(mainPanel, &glowStyle, 0);

  // 🔁 BLOKDBIT Label
  lv_obj_t* blokdbitLabel = lv_label_create(mainPanel);
  lv_label_set_text(blokdbitLabel, "BLOKDBIT");
  lv_obj_set_style_text_color(blokdbitLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(blokdbitLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(blokdbitLabel, LV_ALIGN_TOP_RIGHT, -80, 20);

  // 🔁 SPARK Label
  lv_obj_t* sparkLabel = lv_label_create(mainPanel);
  lv_label_set_text(sparkLabel, "SPARK");
  lv_obj_set_style_text_color(sparkLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(sparkLabel, &lv_font_montserrat_14, 0);
  lv_obj_align(sparkLabel, LV_ALIGN_TOP_RIGHT, -80, 40);

  // 🧩 Widget 1 – Top Left
  lv_obj_t* widget1 = lv_obj_create(mainPanel);
  lv_obj_set_size(widget1, 325, 200);
  lv_obj_align(widget1, LV_ALIGN_TOP_LEFT, 40, 20);
  lv_obj_add_style(widget1, &widget2Style, 0);
  lv_obj_add_style(widget1, &glowStyle, 0);

  // Bitcoin Price Label
  lv_obj_t* priceLabel = lv_label_create(widget1);
  lv_label_set_text(priceLabel, "BITCOIN PRICE");
  lv_obj_set_style_text_color(priceLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(priceLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(priceLabel, LV_ALIGN_TOP_MID, 0, 0);

  // Bitcoin Dollar Value
  priceValueLabel = lv_label_create(widget1); // ✅ fixed: no redeclaration
  lv_label_set_text(priceValueLabel, "Loading...");
  lv_obj_set_style_text_color(priceValueLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(priceValueLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(priceValueLabel, LV_ALIGN_BOTTOM_MID, 0, -10);

  // 🧩 Widget 2 – Top Right
  lv_obj_t* widget2 = lv_obj_create(mainPanel);
  lv_obj_set_size(widget2, 325, 160);
  lv_obj_align(widget2, LV_ALIGN_TOP_RIGHT, -40, 85);
  lv_obj_add_style(widget2, &widget2Style, 0);
  lv_obj_add_style(widget2, &glowStyle, 0);

  // Block Height Label
  lv_obj_t* blockLabel = lv_label_create(widget2);
  lv_label_set_text(blockLabel, "BLOCK HEIGHT");
  lv_obj_set_style_text_color(blockLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(blockLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(blockLabel, LV_ALIGN_TOP_RIGHT, 0, 0);

  // Block Height Number
  blockValueLabel = lv_label_create(widget2); // ✅ fixed: no redeclaration
  lv_label_set_text(blockValueLabel, "Loading...");
  lv_obj_set_style_text_color(blockValueLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(blockValueLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(blockValueLabel, LV_ALIGN_BOTTOM_MID, 0, -10);

// 🧩 Footer Panel (inside mainPanel)
  lv_obj_t* footer = lv_obj_create(mainPanel);
  lv_obj_set_size(footer, 680, 160);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_add_style(footer, &widget2Style, 0);
  lv_obj_add_style(footer, &glowStyle, 0);

  lv_obj_t* feeLabel = lv_label_create(footer);
  lv_label_set_text(feeLabel, "FEE RATES");
  lv_obj_set_style_text_color(feeLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(feeLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(feeLabel, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t* inf = lv_label_create(footer);
  lv_label_set_text(inf, "SPARK v0.01");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
}
