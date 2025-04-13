
// BLOKDBIT_Spark metrics_screen.cpp
// SPARKv0.03

#include <Arduino.h>
#include "metrics_screen.h"
#include "screen_manager.h"

extern String lastPrice;
extern String lastFee;
extern String lastBlockHeight;
extern String lastMiner;


lv_obj_t* priceValueLabel = nullptr;
lv_obj_t* blockValueLabel = nullptr;
lv_obj_t* feeValueLabel = nullptr;
lv_obj_t* solvedByValueLabel = nullptr;
lv_obj_t* leftBtn = nullptr;
lv_obj_t* rightBtn = nullptr;

//extern lv_obj_t* priceValueLabel;
//extern lv_obj_t* blockValueLabel;
//extern lv_obj_t* feeValueLabel;
//extern lv_obj_t* solvedByValueLabel;


extern void onTouchEvent(lv_event_t* e);

bool isLeftOn = false;

// External shared styles and labels 
extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;
extern lv_style_t orangeStyle;
extern lv_style_t greenStyle;
extern lv_style_t blueStyle;


// Handle button press events
void onTouchEvent(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == leftBtn) {
    isLeftOn = !isLeftOn;
    Serial.printf("💚 Left button toggled: %s\n", isLeftOn ? "GREEN" : "ORANGE");
    lv_obj_add_style(leftBtn, isLeftOn ? &greenStyle : &orangeStyle, 0);
  } else if (target == rightBtn) {
    Serial.println("➡️ Right button pressed: Switching to new screen");
    load_screen(1);
  }
}



 // Create and return a new screen

lv_obj_t* create_metrics_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x303030), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);




  // Style Set Up
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
  

 // ---- METRICSpanel ----
  lv_obj_t* metricsPanel = lv_obj_create(scr);
  lv_obj_remove_style_all(metricsPanel);
  lv_obj_set_size(metricsPanel, 760, 440);
  lv_obj_align(metricsPanel, LV_ALIGN_TOP_LEFT, 20, 20);
  lv_obj_set_style_bg_opa(metricsPanel, LV_OPA_TRANSP, 0);
  lv_obj_add_style(metricsPanel, &widgetStyle, 0);
  lv_obj_add_style(metricsPanel, &glowStyle, 0);


  // BLOKDBIT SPARK Labels

  lv_obj_t* blokdbitLabel = lv_label_create(metricsPanel);
  lv_label_set_text(blokdbitLabel, "BLOKDBIT");
  lv_obj_set_style_text_color(blokdbitLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(blokdbitLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(blokdbitLabel, LV_ALIGN_TOP_RIGHT, -80, 20);

  lv_obj_t* sparkLabel = lv_label_create(metricsPanel);
  lv_label_set_text(sparkLabel, "SPARK");
  lv_obj_set_style_text_color(sparkLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(sparkLabel, &lv_font_montserrat_16, 0);
  lv_obj_align(sparkLabel, LV_ALIGN_TOP_RIGHT, -80, 40);


//....WIDGETS...

  // Price Widget
  lv_obj_t* widget1 = lv_obj_create(metricsPanel);
  lv_obj_set_size(widget1, 325, 180);
  lv_obj_align(widget1, LV_ALIGN_TOP_LEFT, 40, 20);
  lv_obj_add_style(widget1, &widget2Style, 0);
  lv_obj_add_style(widget1, &glowStyle, 0);

  lv_obj_t* priceLabel = lv_label_create(widget1);
  lv_label_set_text(priceLabel, "BITCOIN PRICE");
  lv_obj_set_style_text_color(priceLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(priceLabel, &lv_font_unscii_16, 0);
  lv_obj_align(priceLabel, LV_ALIGN_TOP_LEFT, 0, 0);

  priceValueLabel = lv_label_create(widget1);
  lv_label_set_text(priceValueLabel, lastPrice.c_str()); // Update price label
  lv_obj_set_style_text_color(priceValueLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(priceValueLabel, &lv_font_montserrat_30, 0);
  lv_obj_align(priceValueLabel, LV_ALIGN_TOP_LEFT, 0, 20);

  lv_obj_t* feeLabel = lv_label_create(widget1);
  lv_label_set_text(feeLabel, "sat v/B");
  lv_obj_set_style_text_color(feeLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(feeLabel, &lv_font_unscii_16, 0);
  lv_obj_align(feeLabel, LV_ALIGN_TOP_LEFT, 0, 55);
  
  feeValueLabel = lv_label_create(widget1);
  lv_label_set_text(feeValueLabel, lastFee.c_str()); // Update fee label
  lv_obj_set_style_text_color(feeValueLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(feeValueLabel, &lv_font_montserrat_30, 0);
  lv_obj_align(feeValueLabel, LV_ALIGN_TOP_LEFT, 0, 75);


  // Block Height Widget
  lv_obj_t* widget2 = lv_obj_create(metricsPanel);
  lv_obj_set_size(widget2, 325, 160);
  lv_obj_align(widget2, LV_ALIGN_TOP_RIGHT, -40, 85);
  lv_obj_add_style(widget2, &widget2Style, 0);
  lv_obj_add_style(widget2, &glowStyle, 0);

  lv_obj_t* blockLabel = lv_label_create(widget2);
  lv_label_set_text(blockLabel, "BLOCK HEIGHT");
  lv_obj_set_style_text_color(blockLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(blockLabel, &lv_font_unscii_16, 0);
  lv_obj_align(blockLabel, LV_ALIGN_TOP_RIGHT, 0, 0);

  blockValueLabel = lv_label_create(widget2);
  lv_label_set_text(blockValueLabel, lastBlockHeight.c_str()); // Update block height label
  lv_obj_set_style_text_color(blockValueLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(blockValueLabel, &lv_font_montserrat_26, 0);
  lv_obj_align(blockValueLabel, LV_ALIGN_TOP_RIGHT, 0, 20);




  //SOLVED BY Widget
  lv_obj_t* solvedByLabel = lv_label_create(widget2);
  lv_obj_set_style_text_color(solvedByLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(solvedByLabel, &lv_font_unscii_16, 0);
  lv_obj_align(solvedByLabel, LV_ALIGN_TOP_RIGHT, 0, 55);

  solvedByValueLabel = lv_label_create(widget2);
  lv_label_set_text(solvedByValueLabel, lastMiner.c_str()); // Update miner label
  lv_label_set_text(solvedByValueLabel, "MINED By...");
  lv_obj_set_style_text_color(solvedByValueLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(solvedByValueLabel, &lv_font_montserrat_26, 0);
  lv_obj_align(solvedByValueLabel, LV_ALIGN_TOP_RIGHT, 0, 75);





  //****FOOTER***//
  
  lv_obj_t* footer = lv_obj_create(metricsPanel);
  lv_obj_set_size(footer, 680, 160);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_add_style(footer, &widget2Style, 0);
  lv_obj_add_style(footer, &glowStyle, 0);

  

  lv_obj_t* inf = lv_label_create(footer);
  lv_label_set_text(inf, "SPARK v0.01");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  lv_obj_set_style_bg_color(scr, lv_color_hex(0x303030), 0);  // Dark Grey
  //lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);





  // ...NAVIGATION BUTTONS...

   // Left Button
  leftBtn = lv_obj_create(scr);
  lv_obj_set_size(leftBtn, 21, 21);
  lv_obj_align(leftBtn, LV_ALIGN_LEFT_MID, 30, 0);
  lv_obj_add_style(leftBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(leftBtn, onTouchEvent, LV_EVENT_CLICKED, NULL);

  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 21, 21);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -30, 0);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent, LV_EVENT_CLICKED, NULL);


  return scr;
}
