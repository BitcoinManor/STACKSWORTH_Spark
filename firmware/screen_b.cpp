// BLOKDBIT_Spark screen_b.cpp
// SPARKv0.03


#include <Arduino.h>
#include "screen_b.h"
#include "screen_manager.h"


//External shared Objects
extern lv_obj_t* backBtn;


// External shared styles and labels 
extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;
extern lv_style_t orangeStyle;
extern lv_style_t greenStyle;
extern lv_style_t blueStyle;

//bool isRightOn = false;



void onTouchEvent_screen_b(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == backBtn) {
    Serial.println("➡️ Back button pressed: Switching to Metrics Screen");
    load_screen(0);
  }
}


lv_obj_t* create_screen_b() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x303030), 0); //Slate Grey
  //lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

// Main panel
  lv_obj_t* mainPanel = lv_obj_create(scr);
  lv_obj_remove_style_all(mainPanel);
  lv_obj_set_size(mainPanel, 760, 440);
  lv_obj_align(mainPanel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_opa(mainPanel, LV_OPA_TRANSP, 0);
  lv_obj_add_style(mainPanel, &widgetStyle, 0);
  lv_obj_add_style(mainPanel, &glowStyle, 0);



  // Title label

  lv_obj_t* titleLabel = lv_label_create(mainPanel);
  lv_label_set_text(titleLabel, "BLOKDBIT METRICS");
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(titleLabel, &lv_font_unscii_16, 0);  // Safe fallback
  lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);

  
  // 🔷 Mempool Widget
  lv_obj_t* widget1 = lv_obj_create(mainPanel);
  lv_obj_set_size(widget1, 320, 120);
  lv_obj_align(widget1, LV_ALIGN_LEFT_MID, 20, 0);
  lv_obj_add_style(widget1, &widget2Style, 0);
  lv_obj_add_style(widget1, &glowStyle, 0);
  lv_label_set_text(lv_label_create(widget1), "Mempool Size");

  // 🔷 Hashrate Widget
  lv_obj_t* widget2 = lv_obj_create(mainPanel);
  lv_obj_set_size(widget2, 320, 120);
  lv_obj_align(widget2, LV_ALIGN_RIGHT_MID, -20, 0);
  lv_obj_add_style(widget2, &widget2Style, 0);
  lv_obj_add_style(widget2, &glowStyle, 0);
  lv_label_set_text(lv_label_create(widget2), "Network Hashrate");




  //...BUTTONS...

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 21, 21);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 40, 0);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_screen_b, LV_EVENT_CLICKED, NULL);

  

  return scr;
}
