
// STACKSWORTH_Spark world_screen.cpp
// SPARKv0.0.3


#include <Arduino.h>
#include "world_screen.h"
#include "screen_manager.h"
#include "ui_theme.h"
#include <time.h>  // for time(), localtime_r()


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
 
// Cached values (used to populate immediately on screen create)
static String s_time = "--:--";
static String s_city, s_region, s_country;
static int    s_tempC = 0;
static String s_cond;

// Local clock state
static lv_timer_t* s_timeTimer = nullptr;
static bool        s_use12h    = false;   // set true if you want 12h with AM/PM

// Build "HH:MM" (or "hh:mm AM/PM") from ESP32 RTC + current TZ
static void render_time_from_rtc() {
  if (!timeLabel) return;

  time_t now = time(nullptr);
  if (now <= 0) { lv_label_set_text(timeLabel, "--:--"); return; }

  struct tm lt;
  localtime_r(&now, &lt);  // uses global TZ (set via setenv("TZ", ...) + tzset())

  char buf[16];
  if (s_use12h) {
    int hour12 = lt.tm_hour % 12; if (hour12 == 0) hour12 = 12;
    const char* ampm = (lt.tm_hour < 12) ? "AM" : "PM";
    snprintf(buf, sizeof(buf), "%02d:%02d %s", hour12, lt.tm_min, ampm);
  } else {
    snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);
  }
  lv_label_set_text(timeLabel, buf);
}

// Tick every minute so the clock stays correct
static void time_timer_cb(lv_timer_t* /*t*/) {
  render_time_from_rtc();
}




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

  lv_obj_t* inf = lv_label_create(scr);
  lv_label_set_text(inf, "SPARK v0.0.3");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -60, 0);


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
lv_obj_set_width(timeLabel, LV_PCT(100));
lv_obj_set_style_text_align(timeLabel, LV_TEXT_ALIGN_CENTER, 0);
lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_48, 0);
lv_label_set_text(timeLabel, s_time.c_str());     // ← use cached time
render_time_from_rtc();
if (!s_timeTimer) s_timeTimer = lv_timer_create(time_timer_cb, 60000, nullptr);



// Location
locationLabel = lv_label_create(card);
lv_obj_add_style(locationLabel, &ui::st_subtle, 0);
lv_obj_set_width(locationLabel, LV_PCT(100));
lv_obj_set_style_text_align(locationLabel, LV_TEXT_ALIGN_CENTER, 0);
lv_obj_set_style_text_font(locationLabel, &lv_font_montserrat_38, 0);
{
  String line = s_city;
  if (s_region.length())  line += ", " + s_region;
  if (s_country.length()) line += ", " + s_country;
  if (!line.length())     line = "—";
  lv_label_set_text(locationLabel, line.c_str()); // ← cached location
}

// Weather
weatherLabel = lv_label_create(card);
lv_obj_add_style(weatherLabel, &ui::st_accent_secondary, 0);
lv_obj_set_width(weatherLabel, LV_PCT(100));
lv_obj_set_style_text_align(weatherLabel, LV_TEXT_ALIGN_CENTER, 0);
lv_obj_set_style_text_font(weatherLabel, &lv_font_montserrat_30, 0);
if (s_cond.length()) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%d°C — %s", s_tempC, s_cond.c_str());
  lv_label_set_text(weatherLabel, buf);          // ← cached weather
} else {
  lv_label_set_text(weatherLabel, "—");
}


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
  s_time = timeStr;
  if (timeLabel) lv_label_set_text(timeLabel, s_time.c_str());
}

void ui_weather_set_location(const String& city, const String& region, const String& country) {
  s_city = city; s_region = region; s_country = country;
  if (!locationLabel) return;
  String line = s_city;
  if (s_region.length())  line += ", " + s_region;
  if (s_country.length()) line += ", " + s_country;
  if (!line.length())     line = "—";
  lv_label_set_text(locationLabel, line.c_str());
}

void ui_weather_set_current(int tempC, const String& condition) {
  s_tempC = tempC; s_cond = condition;
  if (!weatherLabel) return;
  char buf[64];
  snprintf(buf, sizeof(buf), "%d°C — %s", s_tempC, s_cond.c_str());
  lv_label_set_text(weatherLabel, buf);
}




  
  
