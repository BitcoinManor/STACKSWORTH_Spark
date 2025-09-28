// STACKSWORTH_Spark settings_screen.cpp
// SPARKv0.0.3


#include <Arduino.h>
#include "settings_screen.h"
#include "screen_manager.h"
#include "world_screen.h"
#include <Preferences.h> 

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

//bool isRightOn = false;



static lv_obj_t* g_clockSwitch = nullptr;

static void clock_switch_cb(lv_event_t* e) {
  lv_obj_t* sw = lv_event_get_target(e);
  if (!sw) return;
  bool use12h = lv_obj_has_state(sw, LV_STATE_CHECKED);

  // Persist
  Preferences prefs;
  prefs.begin("cfg", false);
  prefs.putBool("clock12h", use12h);
  prefs.end();

  // Apply immediately (updates world_screen time label)
  ui_weather_set_clock_12h(use12h);
}

// Add a "Clock format (12h)" row with a switch into the given parent container.
static void add_clock_format_setting(lv_obj_t* parent) {
  if (!parent) parent = lv_scr_act();

  // Read current pref (default true → 12h)
  Preferences prefs;
  prefs.begin("cfg", true);
  bool use12h = prefs.getBool("clock12h", true);
  prefs.end();

  // Row container
  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row,
                        LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_hor(row, 8, 0);
  lv_obj_set_style_pad_ver(row, 4, 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

  // Label
  lv_obj_t* lbl = lv_label_create(row);
  lv_label_set_text(lbl, "Clock format (12h)");

  // Switch
  g_clockSwitch = lv_switch_create(row);
  if (use12h) lv_obj_add_state(g_clockSwitch, LV_STATE_CHECKED);
  lv_obj_add_event_cb(g_clockSwitch, clock_switch_cb, LV_EVENT_VALUE_CHANGED, nullptr);
}

// Handle button press events
void onTouchEvent_settings_screen(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == backBtn) {
    //isLeftOn = !isLeftOn;
    Serial.println("➡️ Back button pressed: Switching to Metrics Screen");
    load_screen(3);
  } else if (target == rightBtn) {
    Serial.println("➡️ Right button pressed: Switching to new screen");
    load_screen(0);
  }
}



 



//***Create Screen

lv_obj_t* create_settings_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); //Black
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0); 




  // Title labels

  lv_obj_t* blokdbitLabel = lv_label_create(scr);
  lv_label_set_text(blokdbitLabel, "STACKSWORTH");
  lv_obj_set_style_text_color(blokdbitLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(blokdbitLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(blokdbitLabel, LV_ALIGN_TOP_LEFT, 25, 10);

  lv_obj_t* sparkLabel = lv_label_create(scr);
  lv_label_set_text(sparkLabel, "SPARK");
  lv_obj_set_style_text_color(sparkLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(sparkLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(sparkLabel, LV_ALIGN_TOP_LEFT, 190, 35);

  lv_obj_t* priceheightLabel = lv_label_create(scr);
  lv_label_set_text(priceheightLabel, "SETTINGS");
  lv_obj_set_style_text_color(priceheightLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(priceheightLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(priceheightLabel, LV_ALIGN_TOP_RIGHT, -25, 10);


  add_clock_format_setting(scr);



  //...BUTTONS...

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 25, 20);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_settings_screen, LV_EVENT_CLICKED, NULL);


  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -25, 20);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_settings_screen, LV_EVENT_CLICKED, NULL);
  

  return scr;
}
