// STACKSWORTH_Spark world_screen.h
// SPARKv0.0.3


#pragma once
#include <lvgl.h>
#include <Arduino.h>



// Shared state and controls
extern bool isRightOn;
extern bool isLeftOn;
extern lv_obj_t* backBtn;
extern lv_obj_t* rightBtn;

// Shared event handler
extern void onTouchEvent_world_screen(lv_event_t* e);


// External shared styles and labels 
extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;
extern lv_style_t orangeStyle;
extern lv_style_t greenStyle;
extern lv_style_t blueStyle;

lv_obj_t* create_world_screen();


// UI updaters (called from .ino after fetches)
void ui_weather_set_location(const String& city, const String& region, const String& country);
void ui_weather_set_current(int tempC, const String& condition);
void ui_weather_set_time(const String& timeStr);
void ui_weather_set_tz_label(const String& ianaLabel);
void ui_weather_set_clock_12h(bool use12h);
void ui_weather_set_posix_tz(const String& tz);
