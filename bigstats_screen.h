
// STACKSWORTH_Spark bigstats_screen.h
// SPARKv0.02


#pragma once
#include <lvgl.h>


// Shared state and controls

extern bool isRightOn;
extern lv_style_t orangeStyle;
extern lv_obj_t* backBtn; // ✅ This tells other files it exists
extern lv_obj_t* rightBtn;
extern lv_obj_t* priceValueLabel;
extern lv_obj_t* priceSatsLabel;
extern lv_obj_t* satsUsdLabel;
extern lv_obj_t* satsCadLabel;

extern lv_obj_t* changePillLabel;     // 24h change badge
extern lv_obj_t* priceChartMini;      // mini sparkline
extern lv_chart_series_t* priceSeriesMini;

// Shared event handler
extern void onTouchEvent_bigstats_screen(lv_event_t* e);


// External shared styles and labels 
extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;
extern lv_style_t orangeStyle;
extern lv_style_t greenStyle;
extern lv_style_t blueStyle;

lv_obj_t* create_bigstats_screen();

// Updates the 24h change pill text + style (green/red)
void ui_update_change_pill(float changePct);
