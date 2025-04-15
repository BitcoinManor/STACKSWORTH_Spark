// BLOKDBIT_Spark metrics_screen.h
// SPARKv0.03


#pragma once
#include <lvgl.h>


// Shared state and controls
extern bool isLeftOn;
extern lv_obj_t* priceValueLabel;
extern lv_obj_t* blockValueLabel;
extern lv_obj_t* feeValueLabel;
extern lv_obj_t* solvedByValueLabel;
extern lv_obj_t* backBtn;
extern lv_obj_t* rightBtn;


extern lv_obj_t* priceChart;
extern lv_chart_series_t* priceSeries;



// Shared event handler
extern void onTouchEvent(lv_event_t* e);

// External shared styles and labels 
extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;
extern lv_style_t orangeStyle;
extern lv_style_t greenStyle;
extern lv_style_t blueStyle;

lv_obj_t* create_metrics_screen();
//void initMetricsScreen();
