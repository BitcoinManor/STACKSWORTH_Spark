
// STACKSWORTH_Spark world_screen.cpp
// SPARKv0.0.3


#include <Arduino.h>
#include "world_screen.h"
#include "screen_manager.h"
#include "ui_theme.h"


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


// Handle button press events
void onTouchEvent_world_screen(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == backBtn) {
    //........................................................................................................................................................................................................................................................................................................................................................................................................................isLeftOn = !isLeftOn;
    Serial.println("➡️ Back button pressed: Switching to Big Stats Screen");
    load_screen(1);
  } else if (target == rightBtn) {
    Serial.println("➡️ Right button pressed: Switching to Bitaxe screen");
    load_screen(3);
  }
}


// Handles to labels we’ll update
static lv_obj_t* timeLabel = nullptr;
static lv_obj_t* locationLabel = nullptr;
static lv_obj_t* weatherLabel = nullptr;
 



//***Create Screen

lv_obj_t* create_world_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); //Black
  //lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);




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


  lv_obj_t* mainWeatherLabel = lv_label_create(scr);
  lv_label_set_text(mainWeatherLabel, "WEATHER");
  lv_obj_set_style_text_color(mainWeatherLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(mainWeatherLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(mainWeatherLabel, LV_ALIGN_TOP_RIGHT, -25, 10);


   // Card
  lv_obj_t* card = ui::make_card(scr);
  lv_obj_set_size(card, 660, 360);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(card, 20, 0);
  lv_obj_set_style_pad_row(card, 14, 0);

  // Title
  lv_obj_t* title = lv_label_create(card);
  lv_obj_add_style(title, &ui::st_title, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_label_set_text(title, "TIME • LOCATION • WEATHER");

  // Big time
  timeLabel = lv_label_create(card);
  lv_obj_add_style(timeLabel, &ui::st_value, 0);
  lv_obj_set_width(timeLabel, LV_PCT(100));              // allow center text
  lv_obj_set_style_text_align(timeLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_48, 0);
  lv_label_set_text(timeLabel, "--:--");

  // Location line (City, State/Province, Country)
  locationLabel = lv_label_create(card);
  lv_obj_add_style(locationLabel, &ui::st_subtle, 0);
  lv_obj_set_width(locationLabel, LV_PCT(100));              // allow center text
  lv_obj_set_style_text_align(locationLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(locationLabel, &lv_font_montserrat_38, 0);
  lv_label_set_text(locationLabel, "—");

  // Weather line (e.g., 21°C — Partly Cloudy)
  weatherLabel = lv_label_create(card);
  lv_obj_add_style(weatherLabel, &ui::st_accent_secondary, 0);
  lv_obj_set_width(weatherLabel, LV_PCT(100));              // allow center text
  lv_obj_set_style_text_align(weatherLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(weatherLabel, &lv_font_montserrat_30, 0);
  lv_label_set_text(weatherLabel, "—");

  //...BUTTONS...

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 25, 20);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_world_screen, LV_EVENT_CLICKED, NULL);


  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -25, 20);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_world_screen, LV_EVENT_CLICKED, NULL);

  return scr;
}

void ui_weather_set_time(const String& timeStr) {
  if (timeLabel) lv_label_set_text(timeLabel, timeStr.c_str());
}

void ui_weather_set_location(const String& city, const String& region, const String& country) {
  if (!locationLabel) return;
  String line = city;
  if (region.length())  line += ", " + region;
  if (country.length()) line += ", " + country;
  if (!line.length())   line = "—";
  lv_label_set_text(locationLabel, line.c_str());
}

void ui_weather_set_current(int tempC, const String& condition) {
  if (!weatherLabel) return;
  char buf[64];
  snprintf(buf, sizeof(buf), "%d°C — %s", tempC, condition.c_str());
  lv_label_set_text(weatherLabel, buf);
}



  
  
