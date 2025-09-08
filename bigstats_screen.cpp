// STACKSWORTH_Spark bigstats_screen.cpp
// SPARKv0.02


#include <Arduino.h>
#include "bigstats_screen.h"
#include "screen_manager.h"
#include "ui_theme.h"





extern String lastPrice;


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
void onTouchEvent_bigstats_screen(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == backBtn) {
    //........................................................................................................................................................................................................................................................................................................................................................................................................................isLeftOn = !isLeftOn;
    Serial.println("➡️ Back button pressed: Switching to Metrics Screen");
    load_screen(0);
  } else if (target == rightBtn) {
    Serial.println("➡️ Right button pressed: Switching to World Screen");
    load_screen(2);
  }
}



 



//***Create Screen

lv_obj_t* create_bigstats_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); //Black
  //lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);




   // BLOKDBIT SPARK Labels

  lv_obj_t* blokdbitLabel = lv_label_create(scr);
  lv_label_set_text(blokdbitLabel, "STACKSWORTH");
  lv_obj_set_style_text_color(blokdbitLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(blokdbitLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(blokdbitLabel, LV_ALIGN_TOP_LEFT, 25, 10);

  lv_obj_t* sparkLabel = lv_label_create(scr);
  lv_label_set_text(sparkLabel, "// SPARK");
  lv_obj_set_style_text_color(sparkLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(sparkLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(sparkLabel, LV_ALIGN_TOP_LEFT, 190, 10);


  lv_obj_t* inf = lv_label_create(scr);
  lv_label_set_text(inf, "SPARK v0.02");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -60, 0);

  lv_obj_t* priceheightLabel = lv_label_create(scr);
  lv_label_set_text(priceheightLabel, "Bitcoin Price");
  lv_obj_set_style_text_color(priceheightLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(priceheightLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(priceheightLabel, LV_ALIGN_TOP_RIGHT, -25, 10);


   // ───────────────────────── Price Card (bigstats_screen) ─────────────────────────
lv_obj_t* widget1 = ui::make_card(scr);
lv_obj_set_size(widget1, 640, 400);
// Center the card on the screen
lv_obj_center(widget1);

// Card layout: vertical stack, centered content
lv_obj_set_flex_flow(widget1, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(widget1,
  LV_FLEX_ALIGN_START,   // main axis (top to bottom)
  LV_FLEX_ALIGN_CENTER,  // cross axis (center children)
  LV_FLEX_ALIGN_CENTER   // track cross placement
);
lv_obj_set_style_pad_all(widget1, 20, 0);
lv_obj_set_style_pad_row(widget1, 12, 0);

// ========== Row 1: Title (left) + 24h change pill (right) ==========
lv_obj_t* row1 = lv_obj_create(widget1);
lv_obj_remove_style_all(row1);
lv_obj_set_size(row1, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(row1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
lv_obj_set_style_bg_opa(row1, LV_OPA_TRANSP, 0);

// Title (smaller than metrics screen)
lv_obj_t* priceTitle = lv_label_create(row1);
lv_obj_add_style(priceTitle, &ui::st_title, 0);
lv_label_set_text(priceTitle, "BTC / USD");
lv_obj_set_style_text_font(priceTitle, &lv_font_montserrat_16, 0);

// (Re)usable pill styles (one-time init)
static bool pillInited = false;
static lv_style_t st_pill_up, st_pill_down;
if (!pillInited) {
  pillInited = true;
  lv_style_init(&st_pill_up);
  lv_style_set_radius(&st_pill_up, 999);
  lv_style_set_bg_color(&st_pill_up, lv_color_hex(0x22C55E)); // green
  lv_style_set_bg_opa(&st_pill_up, LV_OPA_COVER);
  lv_style_set_pad_hor(&st_pill_up, 10);
  lv_style_set_pad_ver(&st_pill_up, 4);
  lv_style_set_text_color(&st_pill_up, lv_color_hex(0xFFFFFF));
  lv_style_set_text_font(&st_pill_up, &lv_font_montserrat_14);

  lv_style_init(&st_pill_down);
  lv_style_set_radius(&st_pill_down, 999);
  lv_style_set_bg_color(&st_pill_down, lv_color_hex(0xEF4444)); // red
  lv_style_set_bg_opa(&st_pill_down, LV_OPA_COVER);
  lv_style_set_pad_hor(&st_pill_down, 10);
  lv_style_set_pad_ver(&st_pill_down, 4);
  lv_style_set_text_color(&st_pill_down, lv_color_hex(0xFFFFFF));
  lv_style_set_text_font(&st_pill_down, &lv_font_montserrat_14);
}

// 24h change pill (placeholder until wired)
changePillLabel = lv_label_create(row1);
lv_label_set_text(changePillLabel, "24h: …");
lv_obj_add_style(changePillLabel, &st_pill_down, 0); // default red

// ========== Row 2: Big USD price ==========
priceValueLabel = lv_label_create(widget1);
lv_obj_add_style(priceValueLabel, &ui::st_value, 0);
lv_obj_set_width(priceValueLabel, LV_PCT(100));              // allow center text
lv_obj_set_style_text_align(priceValueLabel, LV_TEXT_ALIGN_CENTER, 0);
lv_obj_set_style_text_font(priceValueLabel, &lv_font_montserrat_48, 0);  // bigger for the big card
lv_label_set_text(priceValueLabel, lastPrice.c_str());

// ========== Row 3: CAD price (subtle, centered) ==========
priceCadLabel = lv_label_create(widget1);
lv_obj_add_style(priceCadLabel, &ui::st_subtle, 0);
lv_obj_set_width(priceCadLabel, LV_PCT(100));
lv_obj_set_style_text_align(priceCadLabel, LV_TEXT_ALIGN_CENTER, 0);
lv_obj_set_style_text_font(priceCadLabel, &lv_font_montserrat_16, 0);
lv_label_set_text(priceCadLabel, "CAD: …");

// ========== Row 4: Mini sparkline (centered, wider) ==========
lv_obj_t* chartRow = lv_obj_create(widget1);
lv_obj_remove_style_all(chartRow);
lv_obj_set_size(chartRow, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_style_bg_opa(chartRow, LV_OPA_TRANSP, 0);
lv_obj_set_flex_flow(chartRow, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(chartRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

priceChartMini = lv_chart_create(chartRow);
lv_obj_set_size(priceChartMini, 560, 60);
lv_obj_set_style_pad_top(priceChartMini, 8, 0);
lv_obj_set_style_pad_bottom(priceChartMini, 8, 0);

lv_chart_set_type(priceChartMini, LV_CHART_TYPE_LINE);
lv_chart_set_point_count(priceChartMini, 24);
lv_chart_set_div_line_count(priceChartMini, 0, 0);
lv_chart_set_update_mode(priceChartMini, LV_CHART_UPDATE_MODE_SHIFT);

// minimal visuals
lv_obj_set_style_border_width(priceChartMini, 0, 0);
lv_obj_set_style_bg_opa(priceChartMini, LV_OPA_TRANSP, 0);
lv_chart_set_range(priceChartMini, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

// Hide markers, keep a thin, rounded line
static lv_style_t st_mini_items;
static bool st_mini_items_inited = false;
if (!st_mini_items_inited) {
  st_mini_items_inited = true;
  lv_style_init(&st_mini_items);
  lv_style_set_size(&st_mini_items, 0);                // no dots
  lv_style_set_line_width(&st_mini_items, 3);          // slightly thicker on big card
  lv_style_set_line_opa(&st_mini_items, LV_OPA_COVER);
  lv_style_set_line_rounded(&st_mini_items, 1);
}
lv_obj_add_style(priceChartMini, &st_mini_items, LV_PART_ITEMS | LV_STATE_ANY);

// series
priceSeriesMini = lv_chart_add_series(priceChartMini, ui::accent_secondary(), LV_CHART_AXIS_PRIMARY_Y);
// seed with flatline
for (int i = 0; i < 24; ++i) priceSeriesMini->y_points[i] = 50;
lv_chart_refresh(priceChartMini);

// ========== Row 5: SATS per $ (side by side, centered) ==========
lv_obj_t* satsRow = lv_obj_create(widget1);
lv_obj_remove_style_all(satsRow);
lv_obj_set_size(satsRow, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_style_bg_opa(satsRow, LV_OPA_TRANSP, 0);
lv_obj_set_flex_flow(satsRow, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(satsRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
lv_obj_set_style_pad_row(satsRow, 0, 0);
lv_obj_set_style_pad_column(satsRow, 24, 0);

satsUsdLabel = lv_label_create(satsRow);
lv_obj_add_style(satsUsdLabel, &ui::st_accent_primary, 0);
lv_obj_set_style_text_font(satsUsdLabel, &lv_font_montserrat_16, 0);
lv_label_set_text(satsUsdLabel, "… SATS / $1 USD");

satsCadLabel = lv_label_create(satsRow);
lv_obj_add_style(satsCadLabel, &ui::st_accent_secondary, 0);
lv_obj_set_style_text_font(satsCadLabel, &lv_font_montserrat_16, 0);
lv_label_set_text(satsCadLabel, "… SATS / $1 CAD");





  //...BUTTONS...

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 40, 20);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_bigstats_screen, LV_EVENT_CLICKED, NULL);


  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -30, 20);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_bigstats_screen, LV_EVENT_CLICKED, NULL);

  return scr;
}




