
// STACKSWORTH_Spark metrics_screen.cpp
// SPARKv0.0.3

#include <Arduino.h>
#include "metrics_screen.h"
#include "screen_manager.h"
#include "ui_theme.h"

extern String lastPrice;
extern String lastFee;
extern String lastBlockHeight;
extern String lastMiner;


lv_obj_t* priceValueLabel = nullptr;
lv_obj_t* priceCadLabel = nullptr;
lv_obj_t* blockValueLabel = nullptr;
lv_obj_t* feeValueLabel = nullptr;
lv_obj_t* solvedByValueLabel = nullptr;
lv_obj_t* satsUsdLabel = nullptr;
lv_obj_t* satsCadLabel = nullptr;
lv_obj_t* changePillLabel = nullptr;
lv_obj_t* priceChartMini = nullptr;
lv_chart_series_t* priceSeriesMini = nullptr;

extern lv_obj_t* backBtn;
extern lv_obj_t* rightBtn;


// Header buttons
lv_obj_t* accentBtn = nullptr;
lv_obj_t* themeBtn  = nullptr;
lv_obj_t* themeBtnLabel = nullptr;
static bool g_lightMode = false;  // start in dark; we’ll toggle

static void ui_apply_theme(bool light) {
  // Baby-step: switch screen background only (we’ll expand later to text/cards)
  lv_obj_t* root = lv_scr_act();
  if (!root) return;

  if (light) {
    lv_obj_set_style_bg_color(root, lv_color_hex(0xFFFFFF), 0); // white
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  } else {
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0); // black
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  }

  // TODO (next baby step): flip common text styles + card bg styles.
  // lv_obj_report_style_change(NULL); // if you change shared styles later
}

static void theme_btn_cb(lv_event_t* e) {
  if (e->code != LV_EVENT_CLICKED) return;
  g_lightMode = !g_lightMode;
  ui_apply_theme(g_lightMode);
  if (themeBtnLabel) lv_label_set_text(themeBtnLabel, g_lightMode ? "☾" : "☀");
}

static void accent_btn_cb(lv_event_t* e) {
  if (e->code != LV_EVENT_CLICKED) return;
  // Placeholder: later we can cycle accent palettes or open a settings screen
  Serial.println("Accent button clicked");
}



// 🔁 Global chart references
lv_chart_series_t* priceSeries = nullptr;
lv_obj_t* priceChart = nullptr;


// --- Fee tier badges (LOW / MED / HIGH) ---
lv_obj_t* feeLowLabel  = nullptr;
lv_obj_t* feeMedLabel  = nullptr;
lv_obj_t* feeHighLabel = nullptr;

// Public helper so the .ino can update all three at once
void ui_update_fee_badges_lmh(int low, int med, int high) {
  if (feeLowLabel)  lv_label_set_text_fmt(feeLowLabel,  "LOW %d",  low);
  if (feeMedLabel)  lv_label_set_text_fmt(feeMedLabel,  "MED %d",  med);
  if (feeHighLabel) lv_label_set_text_fmt(feeHighLabel, "HIGH %d", high);
}

// --- Millionth block hint (split into count + suffix so we can style the number) ---
lv_obj_t* blocksToRow = nullptr;
lv_obj_t* blocksToCountLabel = nullptr;   // gets ui::st_accent_primary
lv_obj_t* blocksToSuffixLabel = nullptr;

void ui_update_blocks_to_million(long height) {
  long remaining = 1000000L - height;
  if (remaining < 0) remaining = 0;

  if (blocksToCountLabel) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%ld", remaining);
    lv_label_set_text(blocksToCountLabel, buf);
  }
  if (blocksToSuffixLabel) {
    lv_label_set_text(blocksToSuffixLabel, " BLOCKS to One Million");
  }
}




//bool isLeftOn = false;

// External shared styles and labels 
extern lv_style_t widgetStyle;
extern lv_style_t widget2Style;
extern lv_style_t glowStyle;
extern lv_style_t orangeStyle;
extern lv_style_t greenStyle;
extern lv_style_t blueStyle;

 
 // Handle button press events
void onTouchEvent_metrics_screen(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == backBtn) {
    //........................................................................................................................................................................................................................................................................................................................................................................................................................isLeftOn = !isLeftOn;
    Serial.println("➡️ Back button pressed: Switching to Settings Screen");
    load_screen(4);
  } else if (target == rightBtn) {
    Serial.println("➡️ Right button pressed: Switching to Big Stats Screen");
    load_screen(1);
  }
}



 // Create and return a new screen

lv_obj_t* create_metrics_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  ui::apply_root_bg(scr);                        // apply new theme background

  //lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);


  // Style Set Up
  lv_style_init(&orangeStyle);
  lv_style_set_bg_color(&orangeStyle, lv_color_hex(0xFFA500));
  lv_style_set_radius(&orangeStyle, 10);
  lv_style_set_bg_opa(&orangeStyle, LV_OPA_COVER);

  lv_style_init(&greenStyle);
  lv_style_set_bg_color(&greenStyle, lv_color_hex(0x00FF00));
  lv_style_set_radius(&greenStyle, 10);
  lv_style_set_bg_opa(&greenStyle, LV_OPA_COVER);

  lv_style_init(&blueStyle);
  lv_style_set_bg_color(&blueStyle, lv_color_hex(0x007BFF));
  lv_style_set_radius(&blueStyle, 10);
  lv_style_set_bg_opa(&blueStyle, LV_OPA_COVER);
  


  // BLOKDBIT SPARK Labels

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

  // Top-right buttons container (keeps buttons together)
  lv_obj_t* hdrBtns = lv_obj_create(scr);
  lv_obj_remove_style_all(hdrBtns);
  lv_obj_set_size(hdrBtns, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(hdrBtns, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(hdrBtns, 8, 0);
  lv_obj_align(hdrBtns, LV_ALIGN_TOP_RIGHT, -25, 10);

  // Accent button (styled)
  accentBtn = lv_btn_create(hdrBtns);
  lv_obj_add_style(accentBtn, &ui::st_accent_primary, 0);
  lv_obj_set_style_radius(accentBtn, 999, 0);
  lv_obj_set_style_pad_hor(accentBtn, 12, 0);
  lv_obj_set_style_pad_ver(accentBtn, 6, 0);
  lv_obj_add_event_cb(accentBtn, accent_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* accentLbl = lv_label_create(accentBtn);
  lv_label_set_text(accentLbl, "ACCENT");

  // Theme toggle (☀ / ☾)
  themeBtn = lv_btn_create(hdrBtns);
  lv_obj_set_style_radius(themeBtn, 999, 0);
  lv_obj_set_style_pad_hor(themeBtn, 12, 0);
  lv_obj_set_style_pad_ver(themeBtn, 6, 0);
  lv_obj_add_event_cb(themeBtn, theme_btn_cb, LV_EVENT_CLICKED, NULL);
  themeBtnLabel = lv_label_create(themeBtn);
  lv_label_set_text(themeBtnLabel, "☀");     // start dark → show sun to indicate "tap for light"



  lv_obj_t* inf = lv_label_create(scr);
  lv_label_set_text(inf, "SPARK v0.0.3");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -60, 0);




//....WIDGETS...


  // ───────────────────────── Price Card (themed + structured) ─────────────────────────
  lv_obj_t* widget1 = ui::make_card(scr);
  lv_obj_set_size(widget1, 240, 190);
  lv_obj_align(widget1, LV_ALIGN_TOP_LEFT, 20, 40);

  // Vertical stack inside the card
  lv_obj_set_flex_flow(widget1, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all (widget1, 12, 0);
  lv_obj_set_style_pad_row (widget1, 8,  0);

  // --- Row 1: Title (left) + 24h change pill (right)
  lv_obj_t* row1 = lv_obj_create(widget1);
  lv_obj_remove_style_all(row1);
  lv_obj_set_size(row1, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_bg_opa(row1, LV_OPA_TRANSP, 0);

  lv_obj_t* priceTitle = lv_label_create(row1);
  lv_obj_add_style(priceTitle, &ui::st_title, 0);
  lv_label_set_text(priceTitle, "BTC / USD");
  // smaller than before
  lv_obj_set_style_text_font(priceTitle, &lv_font_montserrat_14, 0);

  //  pill styles (once)
  static bool pillInited = false;
  static lv_style_t st_pill_up, st_pill_down;
  if (!pillInited) {
    pillInited = true;
    lv_style_init(&st_pill_up);
    lv_style_set_radius(&st_pill_up, 999);
    lv_style_set_bg_color(&st_pill_up, lv_color_hex(0x22C55E)); // green
    lv_style_set_bg_opa(&st_pill_up, LV_OPA_COVER);
    lv_style_set_pad_hor(&st_pill_up, 8);
    lv_style_set_pad_ver(&st_pill_up, 2);
    lv_style_set_text_color(&st_pill_up, lv_color_hex(0xFFFFFF)); // white text on green
    lv_style_set_text_font(&st_pill_up, &lv_font_montserrat_14);

    lv_style_init(&st_pill_down);
    lv_style_set_radius(&st_pill_down, 999);
    lv_style_set_bg_color(&st_pill_down, lv_color_hex(0xEF4444)); // red
    lv_style_set_bg_opa(&st_pill_down, LV_OPA_COVER);
    lv_style_set_pad_hor(&st_pill_down, 8);
    lv_style_set_pad_ver(&st_pill_down, 2);
    lv_style_set_text_color(&st_pill_down, lv_color_hex(0xFFFFFF)); // white text on red
    lv_style_set_text_font(&st_pill_down, &lv_font_montserrat_14);
  }

  // 24h change pill (placeholder until wired)
  changePillLabel = lv_label_create(row1);
  lv_label_set_text(changePillLabel, "24h: …");
  lv_obj_add_style(changePillLabel, &st_pill_down, 0); // default red; we’ll flip style after we compute change

  // --- Row 2: Big USD price (white)
  priceValueLabel = lv_label_create(widget1);
  lv_obj_add_style(priceValueLabel, &ui::st_value, 0);
  lv_label_set_text(priceValueLabel, lastPrice.c_str());
  lv_obj_set_style_text_font(priceValueLabel, &lv_font_montserrat_30, 0);

  // --- Row 3: CAD price (smaller than title)
  priceCadLabel = lv_label_create(widget1);   // reuse global to keep a handle; we'll set text to CAD price for now
  lv_obj_add_style(priceCadLabel, &ui::st_subtle, 0);
  lv_label_set_text(priceCadLabel, "CAD: …");   // we’ll compute and set this after we fetch price/CAD
  lv_obj_set_style_text_font(priceCadLabel, &lv_font_montserrat_14, 0);

  // --- Row 4: mini sparkline (teal line, no dots)
priceChartMini = lv_chart_create(widget1);
lv_obj_set_size(priceChartMini, 210, 30);
lv_obj_set_style_pad_top(priceChartMini, 8, 0);
lv_obj_set_style_pad_bottom(priceChartMini, 8, 0);

lv_chart_set_type(priceChartMini, LV_CHART_TYPE_LINE);
lv_chart_set_point_count(priceChartMini, 24);
lv_chart_set_div_line_count(priceChartMini, 0, 0);
lv_chart_set_update_mode(priceChartMini, LV_CHART_UPDATE_MODE_SHIFT);

// hide all background/border
lv_obj_set_style_border_width(priceChartMini, 0, 0);
lv_obj_set_style_bg_opa(priceChartMini, LV_OPA_TRANSP, 0);

// keep range stable (normalized 0..100 from fetch)
lv_chart_set_range(priceChartMini, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

// add teal series
priceSeriesMini = lv_chart_add_series(priceChartMini, ui::accent_secondary(), LV_CHART_AXIS_PRIMARY_Y);



// Force-hide markers for the mini chart in all states
static lv_style_t st_mini_items;
static bool st_mini_items_inited = false;
if (!st_mini_items_inited) {
  st_mini_items_inited = true;
  lv_style_init(&st_mini_items);
  lv_style_set_size(&st_mini_items, 0);                    // no dots
  lv_style_set_line_width(&st_mini_items, 2);              // thin line
  lv_style_set_line_opa(&st_mini_items, LV_OPA_COVER);     // opaque line
  lv_style_set_line_rounded(&st_mini_items, 1);            // nice caps
}
// Apply to the chart's item part for *all* states
lv_obj_add_style(priceChartMini, &st_mini_items, LV_PART_ITEMS | LV_STATE_ANY);

// Final hammer: force point radius = 0 after styles (overrides any theme)
lv_obj_set_style_size(priceChartMini, 0, LV_PART_ITEMS | LV_STATE_ANY);


// seed with flatline
for (int i = 0; i < 24; ++i) priceSeriesMini->y_points[i] = 50;
lv_chart_refresh(priceChartMini);




  // --- Row 5: SATS per $ lines (accent secondary = teal)
  satsUsdLabel = lv_label_create(widget1);
  lv_obj_add_style(satsUsdLabel, &ui::st_accent_primary, 0);
  lv_label_set_text(satsUsdLabel, "… SATS / $1 USD");
  lv_obj_set_style_text_font(satsUsdLabel, &lv_font_montserrat_14, 0);

  satsCadLabel = lv_label_create(widget1); // keep your existing “CAD” line as accent too
  lv_obj_add_style(satsCadLabel, &ui::st_accent_secondary, 0);
  lv_label_set_text(satsCadLabel, "… SATS / $1 CAD");
  lv_obj_set_style_text_font(satsCadLabel, &lv_font_montserrat_14, 0);





  


// ───────────────────────── Block Height (themed structure only) ─────────────────────────
lv_obj_t* widget2 = ui::make_card(scr);                // themed card container (same as Price/Fees)
lv_obj_set_size(widget2, 240, 190);
lv_obj_align(widget2, LV_ALIGN_TOP_MID, 0, 40);

// Vertical stack inside the card
lv_obj_set_flex_flow(widget2, LV_FLEX_FLOW_COLUMN);
lv_obj_set_style_pad_all(widget2, 12, 0);
lv_obj_set_style_pad_row(widget2, 8, 0);

// Row 1: Title (left) + age pill placeholder (right)
lv_obj_t* blkRow1 = lv_obj_create(widget2);
lv_obj_remove_style_all(blkRow1);
lv_obj_set_size(blkRow1, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(blkRow1, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(blkRow1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

lv_obj_t* blkTitle = lv_label_create(blkRow1);
lv_obj_add_style(blkTitle, &ui::st_title, 0);
lv_obj_set_style_text_font(blkTitle, &lv_font_montserrat_14, 0);
lv_label_set_text(blkTitle, "BLOCK HEIGHT");

// neutral “pill” placeholder (no timing/fetch yet)
lv_obj_t* blkAge = lv_label_create(blkRow1);
lv_obj_set_style_radius(blkAge, 999, 0);
lv_obj_set_style_bg_color(blkAge, lv_color_hex(0x243142), 0); // subtle slate
lv_obj_set_style_bg_opa(blkAge, LV_OPA_COVER, 0);
lv_obj_set_style_pad_hor(blkAge, 8, 0);
lv_obj_set_style_pad_ver(blkAge, 2, 0);
lv_obj_set_style_text_color(blkAge, lv_color_hex(0xCBD5E1), 0);
lv_obj_set_style_text_font(blkAge, &lv_font_montserrat_14, 0);
lv_label_set_text(blkAge, "age —"); // will wire later

// Big height value
blockValueLabel = lv_label_create(widget2);
lv_obj_add_style(blockValueLabel, &ui::st_value, 0);
lv_obj_set_style_text_font(blockValueLabel, &lv_font_montserrat_26, 0);
lv_label_set_text(blockValueLabel, lastBlockHeight.c_str());  // uses your cached value

// Row 3: “Mined By: <pool>”
lv_obj_t* blkRow3 = lv_obj_create(widget2);
lv_obj_remove_style_all(blkRow3);
lv_obj_set_size(blkRow3, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(blkRow3, LV_FLEX_FLOW_ROW);
lv_obj_set_style_pad_column(blkRow3, 6, 0);
lv_obj_set_style_bg_opa(blkRow3, LV_OPA_TRANSP, 0);

lv_obj_t* minedByLabel = lv_label_create(blkRow3);
lv_label_set_text(minedByLabel, "Mined By:");
lv_obj_add_style(minedByLabel, &ui::st_title, 0);
lv_obj_set_style_text_font(minedByLabel, &lv_font_montserrat_14, 0);

solvedByValueLabel = lv_label_create(blkRow3);
lv_obj_add_style(solvedByValueLabel, &ui::st_accent_secondary, 0);
lv_label_set_text(solvedByValueLabel, lastMiner.c_str());  // just the miner name
lv_obj_set_style_text_font(solvedByValueLabel, &lv_font_montserrat_14, 0);

// Hint row: "NNNNNN BLOCKS to the Millionth Block"
blocksToRow = lv_obj_create(widget2);              // widget2 = your Block card container
lv_obj_remove_style_all(blocksToRow);
lv_obj_set_size(blocksToRow, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(blocksToRow, LV_FLEX_FLOW_ROW);
lv_obj_set_style_pad_gap(blocksToRow, 4, 0);
lv_obj_set_style_bg_opa(blocksToRow, LV_OPA_TRANSP, 0);

// Number (accent)
blocksToCountLabel = lv_label_create(blocksToRow);
lv_obj_add_style(blocksToCountLabel, &ui::st_accent_primary, 0);
lv_obj_set_style_text_font(blocksToCountLabel, &lv_font_montserrat_14, 0);
lv_label_set_text(blocksToCountLabel, "--");

// Suffix (subtle)
blocksToSuffixLabel = lv_label_create(blocksToRow);
lv_obj_set_style_text_color(blocksToSuffixLabel, lv_color_hex(0x9AA0A6), 0);
lv_obj_set_style_text_font(blocksToSuffixLabel, &lv_font_montserrat_12, 0);
lv_label_set_text(blocksToSuffixLabel, " BLOCKS to the Millionth Block");





// ───────────────────────── MEMPOOL (themed structure) ─────────────────────────
  // Mempool / Fees (themed + structured)
  lv_obj_t* widget3 = ui::make_card(scr);
  lv_obj_set_size(widget3, 240, 190);
  lv_obj_align(widget3, LV_ALIGN_TOP_RIGHT, -20, 40);

  // Vertical stack
  lv_obj_set_flex_flow(widget3, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all (widget3, 12, 0);
  lv_obj_set_style_pad_row (widget3, 6,  0);

  // Title
  lv_obj_t* mempoolLabel = lv_label_create(widget3);
  lv_obj_add_style(mempoolLabel, &ui::st_title, 0);
  lv_label_set_text(mempoolLabel, "MEMPOOL / FEES");
  lv_obj_set_style_text_font(mempoolLabel, &lv_font_montserrat_14, 0);  // ↑ title size

  // Row: "sat/vB" + value )
  lv_obj_t* feeRow = lv_obj_create(widget3);
  lv_obj_remove_style_all(feeRow);
  lv_obj_set_size(feeRow, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(feeRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(feeRow, 8, 0);
  lv_obj_set_style_bg_opa(feeRow, LV_OPA_TRANSP, 0);

  //lv_obj_t* feeLabel = lv_label_create(feeRow);
  //lv_obj_add_style(feeLabel, &ui::st_title, 0);
  //lv_label_set_text(feeLabel, "sat/vB");
  //lv_obj_set_style_text_font(feeLabel, &lv_font_montserrat_20, 0); // ↑ label size
       
  feeValueLabel = lv_label_create(feeRow);
  lv_obj_add_style(feeValueLabel, &ui::st_value, 0);
  lv_label_set_text(feeValueLabel, lastFee.c_str());
  lv_obj_set_style_text_font(feeValueLabel, &lv_font_montserrat_26, 0); // ↑ value size

// Row: fee tier badges (LOW / MED / HIGH)
lv_obj_t* feeTiersRow = lv_obj_create(widget3);
lv_obj_remove_style_all(feeTiersRow);
lv_obj_set_size(feeTiersRow, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(feeTiersRow, LV_FLEX_FLOW_ROW_WRAP);   // allow wrap if needed
lv_obj_set_style_pad_gap(feeTiersRow, 6, 0);                // was 8
lv_obj_set_style_bg_opa(feeTiersRow, LV_OPA_TRANSP, 0);

// --- LOW (green) ---
feeLowLabel = lv_label_create(feeTiersRow);
lv_obj_set_style_radius(feeLowLabel, 999, 0);
lv_obj_set_style_pad_hor(feeLowLabel, 8, 0);   // was 10
lv_obj_set_style_pad_ver(feeLowLabel, 2, 0);   // was 4
lv_obj_set_style_bg_opa(feeLowLabel, LV_OPA_COVER, 0);
lv_obj_set_style_bg_color(feeLowLabel, lv_color_hex(0x22C55E), 0);
lv_obj_set_style_text_color(feeLowLabel, lv_color_hex(0xFFFFFF), 0);
lv_obj_set_style_text_font(feeLowLabel, &lv_font_montserrat_12, 0);  // smaller font
lv_label_set_text(feeLowLabel, "LOW --");

// --- MED (yellow) ---
feeMedLabel = lv_label_create(feeTiersRow);
lv_obj_set_style_radius(feeMedLabel, 999, 0);
lv_obj_set_style_pad_hor(feeMedLabel, 8, 0);   // was 10
lv_obj_set_style_pad_ver(feeMedLabel, 2, 0);   // was 4
lv_obj_set_style_bg_opa(feeMedLabel, LV_OPA_COVER, 0);
lv_obj_set_style_bg_color(feeMedLabel, lv_color_hex(0xF59E0B), 0);
lv_obj_set_style_text_color(feeMedLabel, lv_color_hex(0x111111), 0);
lv_obj_set_style_text_font(feeMedLabel, &lv_font_montserrat_12, 0);  // smaller font
lv_label_set_text(feeMedLabel, "MED --");

// --- HIGH (red) ---
feeHighLabel = lv_label_create(feeTiersRow);
lv_obj_set_style_radius(feeHighLabel, 999, 0);
lv_obj_set_style_pad_hor(feeHighLabel, 8, 0);  // was 10
lv_obj_set_style_pad_ver(feeHighLabel, 2, 0);  // was 4
lv_obj_set_style_bg_opa(feeHighLabel, LV_OPA_COVER, 0);
lv_obj_set_style_bg_color(feeHighLabel, lv_color_hex(0xEF4444), 0);
lv_obj_set_style_text_color(feeHighLabel, lv_color_hex(0xFFFFFF), 0);
lv_obj_set_style_text_font(feeHighLabel, &lv_font_montserrat_12, 0); // smaller font
lv_label_set_text(feeHighLabel, "HIGH --");


  



  //****FOOTER***//
  
  lv_obj_t* footer = lv_obj_create(scr);
  lv_obj_set_size(footer, 680, 160);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -30);
  lv_obj_add_style(footer, &widget2Style, 0);
  lv_obj_add_style(footer, &glowStyle, 0);


  // 📊 Price Chart Setup
  priceChart = lv_chart_create(footer);
  lv_obj_set_size(priceChart, 640, 50); // Fits footer
  lv_obj_align(priceChart, LV_ALIGN_TOP_MID, 0, 10);
  lv_chart_set_type(priceChart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(priceChart, 24); // 24 data points (hourly)
  lv_chart_set_div_line_count(priceChart, 0, 0);
  lv_chart_set_range(priceChart, LV_CHART_AXIS_PRIMARY_Y, 20000, 80000); // Placeholder BTC range

  
 

  // Optional: make the background more subtle
  lv_obj_set_style_bg_opa(priceChart, LV_OPA_10, 0);

  

  // Optional: make the chart scroll automatically
  lv_chart_set_update_mode(priceChart, LV_CHART_UPDATE_MODE_SHIFT);

  // Series for BTC price line
  priceSeries = lv_chart_add_series(priceChart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

  // Fill with dummy data initially (flatline $30,000)
  for (int i = 0; i < lv_chart_get_point_count(priceChart); i++) {
    priceSeries->y_points[i] = 30000;
  }
  lv_chart_refresh(priceChart);


  

  // Match existing UI styles
  lv_obj_add_style(priceChart, &widget2Style, 0);
  

  

  lv_obj_t* priceChartLabel = lv_label_create(footer);
  lv_label_set_text(priceChartLabel, "24HR Bitcoin Price Chart ");
  lv_obj_set_style_text_color(priceChartLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(priceChartLabel, &lv_font_montserrat_14, 0);
  lv_obj_align(priceChartLabel, LV_ALIGN_BOTTOM_LEFT, 0, 0);



  // ...NAVIGATION BUTTONS...

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 40, 20);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_metrics_screen, LV_EVENT_CLICKED, NULL);


  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -30, 20);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_metrics_screen, LV_EVENT_CLICKED, NULL);


  return scr;
}

// --------------------------------------------------
// --------------------------------------------------
// Updates the 24h change pill text + style (green/red)
void ui_update_change_pill(float changePct) {
    if (!changePillLabel) return;

    // integer-safe formatting (no %f)
    char sign = (changePct >= 0) ? '+' : '-';
    float absPct = changePct >= 0 ? changePct : -changePct;
    int whole = (int)absPct;
    int frac  = (int)((absPct - whole) * 100 + 0.5f);   // 2 decimals, rounded

    lv_label_set_text_fmt(changePillLabel, "24h: %c%d.%02d%%", sign, whole, frac);

    // Flip background color based on sign
    if (changePct >= 0) {
        lv_obj_set_style_bg_color(changePillLabel, lv_color_hex(0x22C55E), 0);
    } else {
        lv_obj_set_style_bg_color(changePillLabel, lv_color_hex(0xEF4444), 0);
    }

    // keep pill styling consistent
    lv_obj_set_style_text_color(changePillLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(changePillLabel, 999, 0);
    lv_obj_set_style_pad_hor(changePillLabel, 8, 0);
    lv_obj_set_style_pad_ver(changePillLabel, 2, 0);
    lv_obj_set_style_bg_opa(changePillLabel, LV_OPA_COVER, 0);
}
