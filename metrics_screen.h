// STACKSWORTH_metrics_screen.h
// SPARKv0.02


#pragma once
#include <Arduino.h>
#include <lvgl.h>


// Shared state and controls
extern bool isLeftOn;
extern lv_obj_t* priceValueLabel;
extern lv_obj_t* priceSatsLabel;
extern lv_obj_t* blockValueLabel;
extern lv_obj_t* feeValueLabel;
extern lv_obj_t* solvedByValueLabel;
extern lv_obj_t* backBtn;
extern lv_obj_t* rightBtn;
// New globals for the Price card
extern lv_obj_t* satsUsdLabel;
extern lv_obj_t* satsCadLabel;

extern lv_obj_t* changePillLabel;     // 24h change badge
extern lv_obj_t* priceChartMini;      // mini sparkline
extern lv_chart_series_t* priceSeriesMini;



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

// Updates the 24h change pill text + style (green/red)
void ui_update_change_pill(float changePct);

// cache + paint helpers so the screen shows instantly on navigation
void ui_cache_price_aux(const String& cadLine,
                        const String& satsUsdLine,
                        const String& satsCadLine);

// Update the 24h low / high / volume pills (USD)
void ui_update_24h_stats(float lowUsd, float highUsd, float volUsd);
                        
