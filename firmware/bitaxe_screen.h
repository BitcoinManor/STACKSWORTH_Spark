// BLOKDBIT_Spark bitaxe_screen.h
// SPARKv0.03

#pragma once
#include <lvgl.h>


// Shared state and controls
extern bool isRightOn;
extern lv_style_t orangeStyle;
extern lv_obj_t* backBtn; // ✅ This tells other files it exists
extern lv_obj_t* leftBtn;
extern lv_obj_t* rightBtn;

// Shared event handler
extern void onTouchEvent_bitaxe_screen(lv_event_t* e);


// External shared styles and labels 
extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;
extern lv_style_t orangeStyle;
extern lv_style_t greenStyle;
extern lv_style_t blueStyle;

lv_obj_t* create_bitaxe_screen();
