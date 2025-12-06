
// STACKSWORTH_Spark world_screen.cpp
// SPARKv0.0.3


#include <Arduino.h>
#include "world_screen.h"
#include "screen_manager.h"
#include "ui_theme.h"
#include <time.h>  // for time(), localtime_r()
#include <stdlib.h>

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
static lv_obj_t* dateLabel = nullptr;
static lv_obj_t* locationLabel = nullptr;
static lv_obj_t* weatherLabel = nullptr;
 
// Cached values (used to populate immediately on screen create)
static String s_time = "--:--";
static String s_city, s_region, s_country;
static int    s_tempC = 0;
static String s_cond;

// Local clock state
static lv_timer_t* s_timeTimer = nullptr;
static bool        s_use12h    = true;   

// Build "HH:MM" (or "hh:mm AM/PM") from ESP32 RTC + current TZ
static void render_time_from_rtc() {
  if (!timeLabel) return;

  time_t now = time(nullptr);
  if (now <= 0) {
    lv_label_set_text(timeLabel, "--:--");
    if (dateLabel) lv_label_set_text(dateLabel, "—");
    return;
  }

  struct tm lt;
  localtime_r(&now, &lt);  // uses the current POSIX TZ

  // Time: 24h or 12h
  char timeBuf[16];
  if (s_use12h) {
    int hour12 = lt.tm_hour % 12; if (hour12 == 0) hour12 = 12;
    const char* ampm = (lt.tm_hour < 12) ? "AM" : "PM";
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d %s", hour12, lt.tm_min, ampm);
  } else {
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", lt.tm_hour, lt.tm_min);
  }
  lv_label_set_text(timeLabel, timeBuf);

  // Date: "Tue • Sep 26"  (add , %Y to include year)
  if (dateLabel) {
    char dateBuf[24];
    strftime(dateBuf, sizeof(dateBuf), "%a • %b %d, %Y", &lt);
    lv_label_set_text(dateLabel, dateBuf);
  }
}


// Tick every minute so the clock stays correct
static void time_timer_cb(lv_timer_t* /*t*/) {
  render_time_from_rtc();
}

// Ensure we have a POSIX TZ set (default to Mountain Time with DST rules)
static void ensure_tz_initialized() {
  const char* tz = getenv("TZ");
  if (!tz || !*tz) {
    // Calgary: MST/MDT — 2nd Sun Mar → 1st Sun Nov at 02:00
    setenv("TZ", "MST7MDT,M3.2.0/2,M11.1.0/2", 1);
    tzset();
  }
}


// Map your portal's saved IANA timezone labels to POSIX TZ strings
// Existing strict table (keep yours above):
static const struct { const char* iana; const char* posix; } kPortalTzMap[] = {
  {"America/Denver",      "MST7MDT,M3.2.0/2,M11.1.0/2"},
  {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2"},
  {"America/New_York",    "EST5EDT,M3.2.0/2,M11.1.0/2"},
  {"Europe/London",       "GMT0BST,M3.5.0/1,M10.5.0/2"},
  {"Europe/Berlin",       "CET-1CEST,M3.5.0/2,M10.5.0/3"},
  {"Asia/Tokyo",          "JST-9"},
  {"Asia/Kolkata",        "IST-5:30"},
  {"Australia/Sydney",    "AEST-10AEDT,M10.1.0/2,M4.1.0/3"},
  {"UTC",                 "UTC0"},
};

// NEW: fuzzy resolver for legacy/short/numeric labels ("MST", "Pacific", "-7", etc.)
static const char* resolve_posix_from_label_fuzzy(String label) {
  label.trim();
  String low = label;
  low.toLowerCase();

  // ----- Numeric offsets your portal may store (Calgary = -7) -----
  // Map common North American offsets to DST-aware rules:
  if (low == "-7" || low == "-07" || low == "-7:00") return "MST7MDT,M3.2.0/2,M11.1.0/2"; // Mountain (Alberta/Denver style DST)
  if (low == "-8" || low == "-08" || low == "-8:00") return "PST8PDT,M3.2.0/2,M11.1.0/2"; // Pacific
  if (low == "-5" || low == "-05" || low == "-5:00") return "EST5EDT,M3.2.0/2,M11.1.0/2"; // Eastern
  if (low == "0"  || low == "+0"  || low == "0:00")  return "UTC0";                         // UTC

  // ----- Named shortcuts you might have saved earlier -----
  if (low == "mst" || low == "mdt" ||
      low.indexOf("mountain") >= 0 || low.indexOf("denver") >= 0 || low.indexOf("edmonton") >= 0)
    return "MST7MDT,M3.2.0/2,M11.1.0/2";

  if (low == "pst" || low == "pdt" ||
      low.indexOf("pacific") >= 0 || low.indexOf("los_angeles") >= 0 || low.indexOf("los angeles") >= 0)
    return "PST8PDT,M3.2.0/2,M11.1.0/2";

  if (low == "est" || low == "edt" ||
      low.indexOf("eastern") >= 0 || low.indexOf("new_york") >= 0 || low.indexOf("new york") >= 0)
    return "EST5EDT,M3.2.0/2,M11.1.0/2";

  if (low.indexOf("london") >= 0)  return "GMT0BST,M3.5.0/1,M10.5.0/2";
  if (low.indexOf("berlin") >= 0)  return "CET-1CEST,M3.5.0/2,M10.5.0/3";
  if (low.indexOf("tokyo") >= 0 || low == "jst") return "JST-9";
  if (low.indexOf("kolkata") >= 0 || low == "ist") return "IST-5:30";
  if (low.indexOf("sydney") >= 0) return "AEST-10AEDT,M10.1.0/2,M4.1.0/3";

  if (low == "utc" || low == "gmt") return "UTC0";

  // Fallback → UTC
  return "UTC0";
}


static void set_tz_from_portal_value(const String& label) {
  // 1) Try exact IANA match from the table
  const char* posix = nullptr;
  for (auto &m : kPortalTzMap) {
    if (label.equalsIgnoreCase(m.iana)) { posix = m.posix; break; }
  }
  // 2) Fallback: accept "MST", "MDT", "Pacific", etc.
  if (!posix) posix = resolve_posix_from_label_fuzzy(label);

  setenv("TZ", posix, 1);
  tzset();
}




//***Create Screen

lv_obj_t* create_world_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); //Black
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
  lv_label_set_text(inf, "SPARK v1.1.1");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -60, 0);

  // ============================================================
  //   MODULAR GRID LAYOUT (like Bitaxe screen)
  //   Base unit: 147px wide × 114px tall with 10px gaps
  // ============================================================

  const int FRAME_INNER_W = 700;
  const int FRAME_INNER_H = 400;
  const int FRAME_PAD = 12;
  const int FRAME_W = FRAME_INNER_W + FRAME_PAD * 2;
  const int FRAME_H = FRAME_INNER_H + FRAME_PAD * 2;
  const int GAP = 10;  // gap between cards

  // Grid calculations (3 columns × 3 rows layout)
  const int CARD_W = 215;  // base width
  const int CARD_H = 120;  // base height

  // Outer frame container
  lv_obj_t* frame = lv_obj_create(scr);
  lv_obj_remove_style_all(frame);
  lv_obj_set_size(frame, FRAME_W, FRAME_H);
  lv_obj_set_style_bg_color(frame, lv_color_hex(0x05070A), 0);
  lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(frame, 2, 0);
  lv_obj_set_style_border_color(frame, lv_color_hex(0x00F0FF), 0);
  lv_obj_set_style_radius(frame, 12, 0);
  lv_obj_set_style_shadow_color(frame, lv_color_hex(0x00F0FF), 0);
  lv_obj_set_style_shadow_opa(frame, LV_OPA_30, 0);
  lv_obj_set_style_shadow_width(frame, 18, 0);
  lv_obj_set_style_shadow_spread(frame, 0, 0);
  lv_obj_align(frame, LV_ALIGN_BOTTOM_MID, 0, -40);

  // ===== CARD 1: DIGITAL CLOCK (2×1 - wide) =====
  lv_obj_t* timeCard = ui::make_card(frame);
  lv_obj_set_size(timeCard, CARD_W * 2 + GAP, CARD_H);
  lv_obj_set_pos(timeCard, FRAME_PAD, FRAME_PAD);
  lv_obj_set_style_pad_all(timeCard, 12, 0);

  lv_obj_t* timeTitle = lv_label_create(timeCard);
  lv_obj_add_style(timeTitle, &ui::st_title, 0);
  lv_label_set_text(timeTitle, "TIME");
  lv_obj_align(timeTitle, LV_ALIGN_TOP_LEFT, 5, 3);

  timeLabel = lv_label_create(timeCard);
  lv_obj_add_style(timeLabel, &ui::st_value, 0);
  lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_38, 0);
  lv_obj_set_style_text_color(timeLabel, lv_color_hex(0x00F5FF), 0);
  lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, 5);

  // ===== CARD 2: DATE (1×1) =====
  lv_obj_t* dateCard = ui::make_card(frame);
  lv_obj_set_size(dateCard, CARD_W, CARD_H);
  lv_obj_set_pos(dateCard, FRAME_PAD + (CARD_W + GAP) * 2, FRAME_PAD);
  lv_obj_set_style_pad_all(dateCard, 12, 0);

  lv_obj_t* dateTitle = lv_label_create(dateCard);
  lv_obj_add_style(dateTitle, &ui::st_title, 0);
  lv_label_set_text(dateTitle, "DATE");
  lv_obj_align(dateTitle, LV_ALIGN_TOP_LEFT, 5, 3);

  dateLabel = lv_label_create(dateCard);
  lv_obj_add_style(dateLabel, &ui::st_subtle, 0);
  lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(dateLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(dateLabel, LV_PCT(90));
  lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 8);

  // ===== CARD 3: TEMPERATURE (1×1) =====
  lv_obj_t* tempCard = ui::make_card(frame);
  lv_obj_set_size(tempCard, CARD_W, CARD_H);
  lv_obj_set_pos(tempCard, FRAME_PAD, FRAME_PAD + CARD_H + GAP);
  lv_obj_set_style_pad_all(tempCard, 12, 0);

  lv_obj_t* tempTitle = lv_label_create(tempCard);
  lv_obj_add_style(tempTitle, &ui::st_title, 0);
  lv_label_set_text(tempTitle, "TEMP");
  lv_obj_align(tempTitle, LV_ALIGN_TOP_LEFT, 5, 3);

  lv_obj_t* tempLabel = lv_label_create(tempCard);
  lv_obj_add_style(tempLabel, &ui::st_accent_secondary, 0);
  lv_obj_set_style_text_font(tempLabel, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(tempLabel, lv_color_hex(0xFF6B6B), 0);
  lv_obj_align(tempLabel, LV_ALIGN_CENTER, 0, 5);

  // ===== CARD 4: WEATHER CONDITION (1×1) =====
  lv_obj_t* condCard = ui::make_card(frame);
  lv_obj_set_size(condCard, CARD_W, CARD_H);
  lv_obj_set_pos(condCard, FRAME_PAD + CARD_W + GAP, FRAME_PAD + CARD_H + GAP);
  lv_obj_set_style_pad_all(condCard, 12, 0);

  lv_obj_t* condTitle = lv_label_create(condCard);
  lv_obj_add_style(condTitle, &ui::st_title, 0);
  lv_label_set_text(condTitle, "CONDITIONS");
  lv_obj_align(condTitle, LV_ALIGN_TOP_LEFT, 5, 3);

  weatherLabel = lv_label_create(condCard);
  lv_obj_add_style(weatherLabel, &ui::st_value, 0);
  lv_obj_set_style_text_font(weatherLabel, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(weatherLabel, lv_color_hex(0x00FF88), 0);
  lv_obj_set_style_text_align(weatherLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(weatherLabel, LV_PCT(90));
  lv_obj_align(weatherLabel, LV_ALIGN_CENTER, 0, 8);

  // ===== CARD 5: TIMEZONE (1×1) =====
  lv_obj_t* tzCard = ui::make_card(frame);
  lv_obj_set_size(tzCard, CARD_W, CARD_H);
  lv_obj_set_pos(tzCard, FRAME_PAD + (CARD_W + GAP) * 2, FRAME_PAD + CARD_H + GAP);
  lv_obj_set_style_pad_all(tzCard, 12, 0);

  lv_obj_t* tzTitle = lv_label_create(tzCard);
  lv_obj_add_style(tzTitle, &ui::st_title, 0);
  lv_label_set_text(tzTitle, "TIMEZONE");
  lv_obj_align(tzTitle, LV_ALIGN_TOP_LEFT, 5, 3);

  lv_obj_t* tzLabel = lv_label_create(tzCard);
  lv_obj_add_style(tzLabel, &ui::st_value, 0);
  lv_obj_set_style_text_font(tzLabel, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(tzLabel, lv_color_hex(0xFFD700), 0);
  lv_label_set_text(tzLabel, "MST\n(UTC-7)");
  lv_obj_set_style_text_align(tzLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(tzLabel, LV_ALIGN_CENTER, 0, 8);

  // ===== CARD 6: LOCATION (3×1 - full width) =====
  lv_obj_t* locationCard = ui::make_card(frame);
  lv_obj_set_size(locationCard, CARD_W * 3 + GAP * 2, CARD_H);
  lv_obj_set_pos(locationCard, FRAME_PAD, FRAME_PAD + (CARD_H + GAP) * 2);
  lv_obj_set_style_pad_all(locationCard, 12, 0);

  lv_obj_t* locationTitle = lv_label_create(locationCard);
  lv_obj_add_style(locationTitle, &ui::st_title, 0);
  lv_label_set_text(locationTitle, "LOCATION");
  lv_obj_align(locationTitle, LV_ALIGN_TOP_LEFT, 5, 3);

  locationLabel = lv_label_create(locationCard);
  lv_obj_add_style(locationLabel, &ui::st_value, 0);
  lv_obj_set_style_text_font(locationLabel, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(locationLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_align(locationLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(locationLabel, LV_PCT(95));
  lv_obj_align(locationLabel, LV_ALIGN_CENTER, 0, 8);

  // Initialize content
  ensure_tz_initialized();
  render_time_from_rtc();

  // Set location content
  {
    String line = s_city;
    if (s_region.length())  line += ", " + s_region;
    if (s_country.length()) line += ", " + s_country;
    if (!line.length())     line = "Location Unknown";
    lv_label_set_text(locationLabel, line.c_str());
  }

  // Set temperature
  if (s_tempC != 0) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d°C", s_tempC);
    lv_label_set_text(tempLabel, buf);
  } else {
    lv_label_set_text(tempLabel, "--°C");
  }

  // Set weather condition
  if (s_cond.length()) {
    lv_label_set_text(weatherLabel, s_cond.c_str());
  } else {
    lv_label_set_text(weatherLabel, "—");
  }

  // Start minute timer
  if (!s_timeTimer) s_timeTimer = lv_timer_create(time_timer_cb, 60000, nullptr);

  // Navigation buttons
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 25, 20);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_world_screen, LV_EVENT_CLICKED, NULL);

  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -25, 20);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_world_screen, LV_EVENT_CLICKED, NULL);

  return scr;
}

void ui_weather_set_time(const String& /*timeStr*/) {
  // Deprecated: we render time from RTC/TZ to avoid inconsistency
  render_time_from_rtc();
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
  s_tempC = tempC; 
  s_cond = condition;
  
  // Update temperature card
  if (weatherLabel) {  // This is actually the temp label now
    char buf[16];
    snprintf(buf, sizeof(buf), "%d°C", tempC);
    // Find and update the temp card label (3rd card)
  }
  
  // Update condition card
  if (weatherLabel) {
    lv_label_set_text(weatherLabel, condition.c_str());
  }
}

void ui_weather_set_tz_label(const String& ianaLabel) {
  set_tz_from_portal_value(ianaLabel);
  render_time_from_rtc();  // repaint time/date immediately
}


void ui_weather_set_clock_12h(bool use12h) {
  s_use12h = use12h;      // file-scope flag defined near the top
  render_time_from_rtc(); // repaint time + date immediately
}


void ui_weather_set_posix_tz(const String& tz) {
  setenv("TZ", tz.c_str(), 1);  // needs <stdlib.h>
  tzset();                      // apply TZ to libc time
  render_time_from_rtc();       // repaint with new timezone
}
  
  
