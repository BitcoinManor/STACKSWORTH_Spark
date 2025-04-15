// BLOKDBIT_Spark world_screen.h
// SPARKv0.03


#pragma once
#include <lvgl.h>

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
