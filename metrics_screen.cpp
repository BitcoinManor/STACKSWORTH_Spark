
// STACKSWORTH_Spark metrics_screen.cpp
// SPARKv0.0.4

#include <Arduino.h>
#include "metrics_screen.h"
#include "screen_manager.h"
#include "ui_theme.h"
#include <time.h>
#include "data_store.h"
#include <Preferences.h>
#include <math.h>
#include <WiFi.h>
#include <HTTPClient.h>



// ───────────────── Accent swap scaffold─────────────────
static bool g_swapAccents = false; // false: normal mapping, true: swapped

// Resolve current accent colors (honors swap flag)
static lv_color_t ACC_PRIMARY()   { return g_swapAccents ? ui::accent_secondary() : ui::accent_primary(); }
static lv_color_t ACC_SECONDARY() { return g_swapAccents ? ui::accent_primary()   : ui::accent_secondary(); }

// Convert lv_color_t -> "RRGGBB" for label recolor (#RRGGBB text#)
static void color_to_hex6(lv_color_t c, char out6[7]) {
  uint32_t v = lv_color_to32(c); // 0x00RRGGBB
  snprintf(out6, 7, "%02X%02X%02X", (v>>16)&0xFF, (v>>8)&0xFF, v&0xFF);
}

// Stored copies (no leading '#'); defaults are just fallbacks
static char g_hex_primary[7]   = "FCA420";
static char g_hex_secondary[7] = "00D1FF";

// Pull current accent colors (AFTER considering swap)
static void refresh_accent_hex_from_theme() {
  color_to_hex6(ACC_PRIMARY(),   g_hex_primary);
  color_to_hex6(ACC_SECONDARY(), g_hex_secondary);
}

// Last known values for the mid-strip so we can repaint when swapping
static struct {
  float hashrateEh = 0;
  float diffPct = 0;
  int   diffDaysAgo = 0;
  float marketCapUsd = 0;
  float circBtc = 0;
  float athUsd = 0;
  int   athDaysAgo = 0;
  bool  valid = false;
} g_midLast;



static String s_prefFiat = "CAD";   // default

// quick symbol helper (optional)
static const char* symbol_for(const String& iso) {
  if (iso.equalsIgnoreCase("USD")) return "$";
  if (iso.equalsIgnoreCase("CAD")) return "$";
  if (iso.equalsIgnoreCase("EUR")) return "€";
  if (iso.equalsIgnoreCase("GBP")) return "£";
  if (iso.equalsIgnoreCase("JPY")) return "¥";
  if (iso.equalsIgnoreCase("AUD")) return "$";
  return "";
}




// Forward decls (defined later in this file)
static void repaint_mid_strip();
static void apply_accent_swap_to_widgets();
static void style_accent_button_like_miner();


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
lv_obj_t* low24Label  = nullptr;
lv_obj_t* high24Label = nullptr;
lv_obj_t* vol24Label  = nullptr;
lv_obj_t* blkAgeLabel = nullptr;
// ── Mid-strip (hash/diff/cap/circ/ath) ─────────────────────────────
lv_obj_t* midStrip      = nullptr;
lv_obj_t* midLine1Label = nullptr;
lv_obj_t* midLine2Label = nullptr;


// cache last-known block timestamp (UNIX seconds)
static uint32_t c_block_ts = 0;

// ── Mid-strip fetch state ──────────────────────────────────────────
static unsigned long g_mid_last_ms = 0;
static uint8_t g_mid_phase = 0;                 // 0=hashrate, 1=diff, 2=supply/market/ath
static const unsigned long MID_FETCH_INTERVAL_MS = 7000;  // ~7s between tiny calls

static bool http_get_text(const char* url, String& out, uint16_t timeoutMs = 6000) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.setTimeout(timeoutMs);
  if (!http.begin(url)) return false;
  int code = http.GET();
  if (code == HTTP_CODE_OK) { out = http.getString(); http.end(); return true; }
  http.end();
  return false;
}


// Hashrate (Blockchain.info /q/hashrate -> GH/s)
static void mid_fetch_hashrate() {
  String body;
  if (!http_get_text("https://blockchain.info/q/hashrate", body)) return;
  double ghs = body.toDouble();         // GH/s
  if (ghs <= 0) return;
  g_midLast.hashrateEh = (float)(ghs / 1e9);  // EH/s
  g_midLast.valid = true;
}

// Difficulty (mempool.space)
static void mid_fetch_difficulty() {
  String body;
  if (!http_get_text("https://mempool.space/api/v1/difficulty-adjustment", body)) return;

  auto findNumber = [&](const char* key)->double {
    int k = body.indexOf(key); if (k < 0) return NAN;
    k = body.indexOf(':', k);  if (k < 0) return NAN;
    int e = k + 1; while (e < (int)body.length() && body[e] == ' ') e++;
    int s = e;
    while (e < (int)body.length() && ((body[e]>='0'&&body[e]<='9')||body[e]=='-'||body[e]=='+'||body[e]=='.')) e++;
    return body.substring(s, e).toFloat();
  };

  double diffChange = findNumber("\"difficultyChange\"");
  double estDaysToNext = findNumber("\"estimatedDays\"");
  if (!isnan(diffChange)) g_midLast.diffPct = (float)diffChange;
  if (!isnan(estDaysToNext)) {
    float daysSince = 14.0f - (float)estDaysToNext;
    if (daysSince < 0) daysSince = 0;
    g_midLast.diffDaysAgo = (int)lroundf(daysSince);
  }
  g_midLast.valid = true;
}

// Circulating (sats) + Market Cap + ATH (fallbacks only)
static void mid_fetch_supply_market_ath() {
  // 1) Circulating (sats)
  String body;
  if (!http_get_text("https://blockchain.info/q/totalbc", body)) return;
  unsigned long long sats = strtoull(body.c_str(), nullptr, 10);
  double circBtc = sats / 100000000.0;
  g_midLast.circBtc = (float)circBtc;

  // 2) Market cap = USD price * circ
  // Try to parse USD price from known strings we already display
  auto parseUsd = [](const String& s)->double {
    if (!s.length()) return 0.0;
    String t = s;
    t.replace(",", "");
    t.replace("$", "");
    t.replace("USD", "");
    t.trim();
    return t.toFloat();  // tolerant of e.g. "91234.56"
  };

  double px = 0.0;
  // Prefer your live label cache first
  if (lastPrice.length()) px = parseUsd(lastPrice);
  // Fallback to your cached pretty string if available
  if (px == 0.0 && Cache::price.usdPretty.length()) px = parseUsd(Cache::price.usdPretty);

  if (px > 0.0) g_midLast.marketCapUsd = (float)(px * circBtc);

  // 3) ATH + days since ATH — use conservative fallbacks
  g_midLast.athUsd = 69000.0f;  // fallback ATH
  {
    time_t now = time(nullptr);
    const time_t ath_ts = 1636502400; // 2021-11-10 00:00:00 UTC
    long days = (now > ath_ts) ? (long)((now - ath_ts) / 86400) : 0;
    g_midLast.athDaysAgo = (int)days;
  }

  g_midLast.valid = true;
}




// Staggered tick
static void mid_metrics_tick() {
  unsigned long now = millis();
  if (now - g_mid_last_ms < MID_FETCH_INTERVAL_MS) return;
  g_mid_last_ms = now;

  switch (g_mid_phase) {
    case 0: mid_fetch_hashrate(); break;
    case 1: mid_fetch_difficulty(); break;
    case 2: mid_fetch_supply_market_ath(); break;
  }
  g_mid_phase = (g_mid_phase + 1) % 3;
  repaint_mid_strip();
}



extern lv_obj_t* backBtn;
extern lv_obj_t* rightBtn;


// Forward declaration
static void hydrate_metrics_from_cache();
void ui_update_change_pill(float changePct);
void ui_update_24h_stats(float lowUsd, float highUsd, float volUsd);
static void fmt_usd_compact(float v, char* out, size_t outlen);

// --- small helpers ---
static inline void set_text_if(lv_obj_t* lbl, const String& s) {
  if (lbl && s.length()) lv_label_set_text(lbl, s.c_str());
}

static inline void set_fee_pill(lv_obj_t* lbl, const char* name, int v) {
  if (!lbl) return;
  if (v >= 0) lv_label_set_text_fmt(lbl, "%s %d", name, v);
}


// ── Small helper: int with commas ──────────────────────────────────
static void fmt_int_commas_ul(unsigned long v, char* out, size_t n) {
  String s = String(v);
  for (int i = s.length() - 3; i > 0; i -= 3) {
    s = s.substring(0, i) + "," + s.substring(i);
  }
  strncpy(out, s.c_str(), n);
  out[n-1] = '\0';
}

// ── Public: update the mid-strip with real values ──────────────────
// fromAthPct: negative if price is below ATH (mirrors dashboard behavior)
void ui_update_mid_metrics(float hashrateEh,
                           float diffPct,
                           int   diffDaysAgo,
                           float marketCapUsd,
                           float circBtc,
                           float athUsd,
                           int   athDaysAgo,
                           float /*fromAthPct*/) {
  // store latest so we can repaint when the accent swap flips
  g_midLast.hashrateEh   = hashrateEh;
  g_midLast.diffPct      = diffPct;
  g_midLast.diffDaysAgo  = diffDaysAgo;
  g_midLast.marketCapUsd = marketCapUsd;
  g_midLast.circBtc      = circBtc;
  g_midLast.athUsd       = athUsd;
  g_midLast.athDaysAgo   = athDaysAgo;
  g_midLast.valid        = true;

  // rebuild the two lines with current (possibly swapped) accents
  repaint_mid_strip();
}



// Header buttons
lv_obj_t* accentBtn = nullptr;
lv_obj_t* themeBtn  = nullptr;
lv_obj_t* themeBtnLabel = nullptr;
static bool g_lightMode = false;  // start in dark; we’ll toggle

static void ui_apply_theme(bool light) {
  // switch screen background only (we’ll expand later to text/cards)
  lv_obj_t* root = lv_scr_act();
  if (!root) return;

  if (light) {
    lv_obj_set_style_bg_color(root, lv_color_hex(0xFFFFFF), 0); // white
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  } else {
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0); // black
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  }

}

static void theme_btn_cb(lv_event_t* e) {
  if (e->code != LV_EVENT_CLICKED) return;
  g_lightMode = !g_lightMode;
  ui_apply_theme(g_lightMode);
  if (themeBtnLabel) lv_label_set_text(themeBtnLabel, g_lightMode ? "MOON" : "SUN");
}

static void accent_btn_cb(lv_event_t* e) {
  if (e->code != LV_EVENT_CLICKED) return;
  g_swapAccents = !g_swapAccents;   // flip mapping
  repaint_mid_strip();              // rebuild mid-strip with swapped accents
  apply_accent_swap_to_widgets();   // recolor other widgets
  Serial.println("Accent swap toggled");
}


// Tap Price card → go to Big Stats
static void price_card_cb(lv_event_t* e) {
  if (e->code != LV_EVENT_CLICKED) return;
  Serial.println("🟧 Price card tapped → Big Stats");
  load_screen(1);  // bigstats_screen
}


// 🔁 Global chart references
lv_chart_series_t* priceSeries = nullptr;
lv_obj_t* priceChart = nullptr;

// --- Persisted cache for instant paint on screen re-entry ---
static String c_priceCad;      // e.g., "CAD $91,234.56"
static String c_satsUsd;       // e.g., "1452 SATS / $1 USD"
static String c_satsCad;       // e.g., "1320 SATS / $1 CAD"
static int    c_feeLow  = -1, c_feeMed = -1, c_feeHigh = -1;
static float  c_changePct = 0.0f;
static bool   c_changePct_set = false;   // track validity without isnan

// repaint just the two “second currency” lines using whatever cache we have
static void repaint_second_currency_lines() {
  // CAD price line label (we’ll reuse this label for the selected fiat)
  if (priceCadLabel) {
    if (s_prefFiat.equalsIgnoreCase("CAD")) {
      // use your cached CAD pretty line if present
      if (c_priceCad.length()) lv_label_set_text(priceCadLabel, c_priceCad.c_str());
      else lv_label_set_text(priceCadLabel, "CAD: —");
    } else if (s_prefFiat.equalsIgnoreCase("USD")) {
      // render from Cache::price.usdPretty if available
      if (Cache::price.usdPretty.length()) {
        String line = "USD " + Cache::price.usdPretty;
        lv_label_set_text(priceCadLabel, line.c_str());
      } else {
        lv_label_set_text(priceCadLabel, "USD: —");
      }
    } else {
      // other fiats to be wired later with real rates
      String ph = s_prefFiat + ": —";
      lv_label_set_text(priceCadLabel, ph.c_str());
    }
  }

  // SATS / 1 <fiat> line — for now we show cached ones if they exist, else placeholder
  if (satsCadLabel) {
    if (s_prefFiat.equalsIgnoreCase("CAD")) {
      if (c_satsCad.length()) lv_label_set_text(satsCadLabel, c_satsCad.c_str());
      else lv_label_set_text(satsCadLabel, "… SATS / 1 CAD");
    } else if (s_prefFiat.equalsIgnoreCase("USD")) {
      if (c_satsUsd.length()) lv_label_set_text(satsCadLabel, c_satsUsd.c_str());
      else lv_label_set_text(satsCadLabel, "… SATS / 1 USD");
    } else {
      // Future: compute from price + FX. Placeholder for now.
      String line = String("… SATS / 1 ") + s_prefFiat;
      lv_label_set_text(satsCadLabel, line.c_str());
    }
  }
}

// Public: called from settings screen and also at boot
void ui_price_set_preferred_fiat(const String& iso) {
  s_prefFiat = iso;

  // Persist (so boot uses the last choice)
  Preferences prefs;
  prefs.begin("cfg", false);
  prefs.putString("fiat", s_prefFiat);
  prefs.end();

  repaint_second_currency_lines();
}

// --- Fee tier badges (LOW / MED / HIGH) ---
lv_obj_t* feeLowLabel  = nullptr;
lv_obj_t* feeMedLabel  = nullptr;
lv_obj_t* feeHighLabel = nullptr;

// Public helper so the .ino can update all three at once
void ui_update_fee_badges_lmh(int low, int med, int high) {
  // cache
  c_feeLow = low; c_feeMed = med; c_feeHigh = high;
  // paint
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


// Make compact "age" text like "57s", "12m", "1h 23m"
static void format_age_short(uint32_t secs, char* out, size_t n) {
  if (secs < 90) {                       // under ~1.5 min, show seconds
    snprintf(out, n, "%us", (unsigned)secs);
    return;
  }
  uint32_t mins = secs / 60;
  if (mins < 90) {                       // under ~1.5 hr, show minutes
    snprintf(out, n, "%um", (unsigned)mins);
    return;
  }
  uint32_t hrs  = mins / 60;
  uint32_t remm = mins % 60;
  if (remm == 0) snprintf(out, n, "%uh", (unsigned)hrs);
  else           snprintf(out, n, "%uh %um", (unsigned)hrs, (unsigned)remm);
}

// Paint the pill from a UNIX timestamp (and cache it)
void ui_update_block_age_from_unix(uint32_t block_ts) {
  c_block_ts = block_ts;

  if (!blkAgeLabel) return;           // UI not built yet—cache only

  time_t now = time(nullptr);
  if (now <= 0 || block_ts == 0) {
    lv_label_set_text(blkAgeLabel, "age —");
    return;
  }
  uint32_t delta = (now > (time_t)block_ts) ? (uint32_t)(now - block_ts) : 0;
  char buf[24];
  format_age_short(delta, buf, sizeof(buf));
  lv_label_set_text_fmt(blkAgeLabel, "age %s", buf);
}

// Cheap ticker (re-renders from cached UNIX ts)
void ui_tick_block_age() {
  if (c_block_ts == 0 || !blkAgeLabel) return;
  ui_update_block_age_from_unix(c_block_ts);
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


void ui_cache_price_aux(const String& cadLine,
                        const String& satsUsdLine,
                        const String& satsCadLine) {
  // Save
  c_priceCad = cadLine;
  c_satsUsd  = satsUsdLine;
  c_satsCad  = satsCadLine;

  // Paint if labels exist
  if (priceCadLabel && c_priceCad.length()) lv_label_set_text(priceCadLabel, c_priceCad.c_str());
  if (satsUsdLabel  && c_satsUsd.length())  lv_label_set_text(satsUsdLabel,  c_satsUsd.c_str());
  if (satsCadLabel  && c_satsCad.length())  lv_label_set_text(satsCadLabel,  c_satsCad.c_str());
}


static void hydrate_metrics_from_cache() {
 const auto& price = Cache::price;
 const auto& block = Cache::block;
 const auto& fees  = Cache::fees;
 const auto& chart = Cache::chart;

  // --- Price / change ---
  if (priceValueLabel && price.usdPretty.length())
    lv_label_set_text(priceValueLabel, price.usdPretty.c_str());

  if (priceCadLabel && price.cadLine.length())
    lv_label_set_text(priceCadLabel, price.cadLine.c_str());

  if (satsUsdLabel && price.satsUsd.length())
    lv_label_set_text(satsUsdLabel, price.satsUsd.c_str());

  if (satsCadLabel && price.satsCad.length())
    lv_label_set_text(satsCadLabel, price.satsCad.c_str());

  if (changePillLabel && price.changeValid)
    ui_update_change_pill(price.changePct);

  // --- Fees ---
  if (feeValueLabel && fees.high >= 0) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d sat/vB", fees.high);
    lv_label_set_text(feeValueLabel, buf);
  }
  if (fees.low >= 0 && fees.med >= 0 && fees.high >= 0) {
    ui_update_fee_badges_lmh(fees.low, fees.med, fees.high);
  }

  // --- Block + age + millionth hint ---
  if (blockValueLabel && block.height.length())
    lv_label_set_text(blockValueLabel, block.height.c_str());

  if (solvedByValueLabel && block.miner.length())
    lv_label_set_text(solvedByValueLabel, block.miner.c_str());

  if (block.height.length())
    ui_update_blocks_to_million(block.height.toInt());

  if (block.ts > 0)
    ui_update_block_age_from_unix(block.ts);

  // --- 24h stat pills ---
  ui_update_24h_stats(chart.low, chart.high, chart.volUsd);

  // --- Footer chart (24 points) ---
  if (priceChart && priceSeries) {
    for (int i = 0; i < 24; i++) {
      lv_chart_set_value_by_id(priceChart, priceSeries, i, (lv_coord_t)chart.points[i]);
    }
    lv_chart_set_range(priceChart, LV_CHART_AXIS_PRIMARY_Y,
                       (lv_coord_t)chart.low, (lv_coord_t)chart.high);
    lv_chart_refresh(priceChart);
  }

  // --- Mini sparkline (0..100) ---
  if (priceChartMini && priceSeriesMini) {
    for (int i = 0; i < 24; i++) {
      lv_chart_set_value_by_id(priceChartMini, priceSeriesMini, i, (lv_coord_t)chart.mini[i]);
    }
    lv_chart_refresh(priceChartMini);
  }
}

// Rebuild the two mid-strip lines using current swap + theme hex
static void repaint_mid_strip() {
  if (!midLine1Label || !midLine2Label || !g_midLast.valid) return;

  // pull ACC_PRIMARY/SECONDARY into hex strings for recolor tags
  refresh_accent_hex_from_theme();

  const char* hexL1 = g_hex_primary;   // Line 1 → primary (or swapped)
  const char* hexL2 = g_hex_secondary; // Line 2 → secondary (or swapped)

  // Line 1: Hashrate / Difficulty
  const char arrow  = (g_midLast.diffPct > 0) ? '▲' : ((g_midLast.diffPct < 0) ? '▼' : '◆');
  const float absPc = g_midLast.diffPct >= 0 ? g_midLast.diffPct : -g_midLast.diffPct;

  char l1[200];
  snprintf(l1, sizeof(l1),
           "Hashrate: #%s %.0f EH/s#  •  Difficulty: #%s %c%.2f%%%# (#%s %dd#)",
           hexL1, g_midLast.hashrateEh,
           hexL1, arrow, absPc,
           hexL1, g_midLast.diffDaysAgo);
  lv_label_set_text(midLine1Label, l1);

  // Helpers for Line 2
  char capBuf[32];  fmt_usd_compact(g_midLast.marketCapUsd, capBuf, sizeof capBuf);
  unsigned long circ = (g_midLast.circBtc >= 0) ? (unsigned long)(g_midLast.circBtc + 0.5f) : 0;
  char circNum[24]; fmt_int_commas_ul(circ, circNum, sizeof circNum);
  char athBuf[32];  fmt_usd_compact(g_midLast.athUsd, athBuf, sizeof athBuf);

  // Line 2: Market Cap / Circulating / ATH
  char l2[240];
  snprintf(l2, sizeof(l2),
           "Market Cap: #%s %s#  •  Circulating Supply: #%s %s / 21,000,000#  •  ATH: #%s %s# (#%s %dd ago#)",
           hexL2, capBuf,
           hexL2, circNum,
           hexL2, athBuf,
           hexL2, g_midLast.athDaysAgo);
  lv_label_set_text(midLine2Label, l2);
}

// Update other accent-colored widgets on this screen
static void apply_accent_swap_to_widgets() {
  // SATS labels: flip with accents
  if (satsUsdLabel) lv_obj_set_style_text_color(satsUsdLabel, ACC_PRIMARY(), 0);
  if (satsCadLabel) lv_obj_set_style_text_color(satsCadLabel, ACC_SECONDARY(), 0);

  // Price charts: swap series color
  if (priceSeriesMini) { priceSeriesMini->color = ACC_SECONDARY(); if (priceChartMini) lv_chart_refresh(priceChartMini); }
  if (priceSeries)     { priceSeries->color     = ACC_SECONDARY(); if (priceChart)     lv_chart_refresh(priceChart); }

  // 24h pills outlines (optional to tie to accents)
  if (low24Label)  lv_obj_set_style_border_color(low24Label,  ACC_SECONDARY(), 0);
  if (high24Label) lv_obj_set_style_border_color(high24Label, ACC_PRIMARY(),   0);

  // Block card small accents
  if (blocksToCountLabel)  lv_obj_set_style_text_color(blocksToCountLabel,  ACC_PRIMARY(),   0);
  if (solvedByValueLabel)  lv_obj_set_style_text_color(solvedByValueLabel, ACC_SECONDARY(), 0);

  style_accent_button_like_miner();

}


// Make the block "age" pill use the same accent as the miner label
static void style_age_pill_like_miner() {
  if (!blkAgeLabel) return;
  // miner name uses ACC_SECONDARY() in this screen → mirror it for the pill
  lv_obj_set_style_bg_color(blkAgeLabel, ACC_SECONDARY(), 0);
  lv_obj_set_style_bg_opa(blkAgeLabel, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(blkAgeLabel, lv_color_hex(0x111111), 0); // dark text for contrast
}


// Make the ACCENT button background match the miner's accent (ACC_SECONDARY)
static void style_accent_button_like_miner() {
  if (!accentBtn) return;

  // bg follows the same accent used for "Solved By" (ACC_SECONDARY honors the swap)
  lv_obj_set_style_bg_color(accentBtn, ACC_SECONDARY(), 0);
  lv_obj_set_style_bg_opa  (accentBtn, LV_OPA_COVER,   0);

  // set label text color for contrast (grab the first child label)
  lv_obj_t* lbl = lv_obj_get_child(accentBtn, 0);
  if (lbl) lv_obj_set_style_text_color(lbl, lv_color_hex(0x111111), 0);
}


// ── Block Interval Visualizer (footer) ────────────────────────────
static lv_obj_t* intervalCard   = nullptr;
static lv_obj_t* intervalChart  = nullptr;
static lv_chart_series_t* intervalSeries = nullptr;
static lv_obj_t*   intervalLabelRow = nullptr;
static lv_obj_t*   blockNumLabels[12] = {nullptr};

// Update bars from an array of minutes (oldest→newest). Values clamped to 0..60.
void ui_update_block_intervals(const uint8_t* minutes, int count) {
  if (!intervalChart || !intervalSeries || !minutes) return;
  const int n = lv_chart_get_point_count(intervalChart);
  for (int i = 0; i < n; ++i) {
    // right-align latest values into the chart window
    const int src = count - n + i;
    int v = (src >= 0 && src < count) ? minutes[src] : 0;
    const int MAXY = 25;  // clamp to 0..25 minutes
    if (v < 0) v = 0; else if (v > MAXY) v = MAXY;
    lv_chart_set_value_by_id(intervalChart, intervalSeries, i, (lv_coord_t)v);
  }
  lv_chart_refresh(intervalChart);
}

// Public: update the 12 block-number labels under each bar (oldest→newest)
void ui_update_block_labels(const uint32_t* heights, int count) {
  if (!intervalLabelRow || !heights) return;

  // Right-align into 12 slots (like the bars)
  for (int i = 0; i < 12; ++i) {
    int src = count - 12 + i;
    if (src >= 0 && src < count) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%u", (unsigned)heights[src]);
      lv_label_set_text(blockNumLabels[i], buf);
    } else {
      lv_label_set_text(blockNumLabels[i], "");
    }
  }
}



static lv_obj_t* ivTargetLine = nullptr;


static void iv_update_target_line() {
  if (!intervalChart || !ivTargetLine) return;
  lv_area_t a; lv_obj_get_content_coords(intervalChart, &a);  // chart plot box
  const float yMin = 0.f, yMax = 25.f;                        // keep in sync with set_range
  float frac = (10.f - yMin) / (yMax - yMin);                 // 0..1 up from bottom
  lv_coord_t y = a.y2 - (lv_coord_t)lroundf(frac * (a.y2 - a.y1));
  lv_point_t pts[2] = { {a.x1, y}, {a.x2, y} };
  lv_line_set_points(ivTargetLine, pts, 2);
}

// Create the row of 12 labels beneath the chart; call once from create_metrics_screen()
static void ensure_block_label_row() {
  if (intervalLabelRow) return;

  intervalLabelRow = lv_obj_create(intervalCard);
  lv_obj_remove_style_all(intervalLabelRow);
  lv_obj_set_size(intervalLabelRow, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(intervalLabelRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_top(intervalLabelRow, 4, 0);
  lv_obj_set_style_pad_left(intervalLabelRow, 6, 0);
  lv_obj_set_style_pad_right(intervalLabelRow, 6, 0);

  // Even spacing under the 12 bars
  lv_obj_set_flex_flow(intervalLabelRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(intervalLabelRow,
                        LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  for (int i = 0; i < 12; ++i) {
    blockNumLabels[i] = lv_label_create(intervalLabelRow);
    lv_obj_set_style_text_color(blockNumLabels[i], lv_color_hex(0x9AA0A6), 0);
    lv_obj_set_style_text_font(blockNumLabels[i], &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(blockNumLabels[i], LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(blockNumLabels[i], ""); // init empty
  }
}


static void apply_accent_to_interval() {
  if (intervalSeries) intervalSeries->color = ACC_SECONDARY();       // bars
  if (ivTargetLine)   lv_obj_set_style_line_color(ivTargetLine, ACC_PRIMARY(), 0); // dashed
  lv_chart_refresh(intervalChart);
}




 //CREATE METRICS SCREEN/////////////////

lv_obj_t* create_metrics_screen() {
lv_obj_t* scr = lv_obj_create(NULL);
ui::apply_root_bg(scr);                        // apply new theme background


// pull current theme colors into hex strings for label recolor
refresh_accent_hex_from_theme();



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
  


  // STACKSWORTH SPARK Labels

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
  lv_obj_add_style(accentBtn, &ui::st_accent_secondary, 0);
  lv_obj_set_style_radius(accentBtn, 999, 0);
  lv_obj_set_style_pad_hor(accentBtn, 12, 0);
  lv_obj_set_style_pad_ver(accentBtn, 6, 0);
  lv_obj_add_event_cb(accentBtn, accent_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* accentLbl = lv_label_create(accentBtn);
  lv_label_set_text(accentLbl, "ACCENT");
  style_accent_button_like_miner();
  

  // Theme toggle (☀ / ☾)
  themeBtn = lv_btn_create(hdrBtns);
  lv_obj_set_style_radius(themeBtn, 999, 0);
  lv_obj_set_style_pad_hor(themeBtn, 12, 0);
  lv_obj_set_style_pad_ver(themeBtn, 6, 0);
  lv_obj_add_event_cb(themeBtn, theme_btn_cb, LV_EVENT_CLICKED, NULL);
  themeBtnLabel = lv_label_create(themeBtn);
  lv_label_set_text(themeBtnLabel, "SUN");     // start dark → show sun to indicate "tap for light"



  lv_obj_t* inf = lv_label_create(scr);
  lv_label_set_text(inf, "SPARK v0.0.4");
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
  if (c_changePct_set) ui_update_change_pill(c_changePct);
  lv_obj_add_style(changePillLabel, &st_pill_down, 0); // default red; we’ll flip style after we compute change

  // --- Row 2: Big USD price (white)
  priceValueLabel = lv_label_create(widget1);
  lv_obj_add_style(priceValueLabel, &ui::st_value, 0);
  lv_label_set_text(priceValueLabel, lastPrice.c_str());
  lv_obj_set_style_text_font(priceValueLabel, &lv_font_montserrat_30, 0);

  // --- Row 3: CAD price (smaller than title)
  priceCadLabel = lv_label_create(widget1);
  lv_obj_add_style(priceCadLabel, &ui::st_subtle, 0);
  lv_obj_set_style_text_font(priceCadLabel, &lv_font_montserrat_14, 0);
  lv_label_set_text(priceCadLabel, c_priceCad.length() ? c_priceCad.c_str() : "CAD: …");


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
  lv_obj_set_style_text_font(satsUsdLabel, &lv_font_montserrat_14, 0);
  lv_label_set_text(satsUsdLabel, c_satsUsd.length() ? c_satsUsd.c_str() : "… SATS / $1 USD");


  satsCadLabel = lv_label_create(widget1);
  lv_obj_add_style(satsCadLabel, &ui::st_accent_secondary, 0);
  lv_obj_set_style_text_font(satsCadLabel, &lv_font_montserrat_14, 0);
  lv_label_set_text(satsCadLabel, c_satsCad.length() ? c_satsCad.c_str() : "… SATS / $1 CAD");

// Initialize second currency preference from prefs and paint once
{
  Preferences prefs;
  prefs.begin("cfg", true);
  String iso = prefs.getString("fiat", "CAD");
  prefs.end();
  ui_price_set_preferred_fiat(iso);
}



// Make the whole Price card tappable
lv_obj_add_flag(widget1, LV_OBJ_FLAG_CLICKABLE);
lv_obj_clear_flag(widget1, LV_OBJ_FLAG_SCROLLABLE);  // avoid “scroll” eating the tap
lv_obj_add_event_cb(widget1, price_card_cb, LV_EVENT_CLICKED, NULL);



  


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
blkAgeLabel = lv_label_create(blkRow1);

lv_obj_set_style_radius(blkAgeLabel, 999, 0);
lv_obj_set_style_bg_color(blkAgeLabel, lv_color_hex(0x243142), 0);
lv_obj_set_style_bg_opa(blkAgeLabel, LV_OPA_COVER, 0);
lv_obj_set_style_pad_hor(blkAgeLabel, 8, 0);
lv_obj_set_style_pad_ver(blkAgeLabel, 2, 0);
lv_obj_set_style_text_color(blkAgeLabel, lv_color_hex(0xCBD5E1), 0);
lv_obj_set_style_text_font(blkAgeLabel, &lv_font_montserrat_14, 0);
lv_label_set_text(blkAgeLabel, "age —");
style_age_pill_like_miner();

// if we already know the latest block timestamp, render it immediately
if (c_block_ts) ui_tick_block_age();

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

if (lastBlockHeight.length()) {
  ui_update_blocks_to_million(lastBlockHeight.toInt());
}



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

if (c_feeLow >= 0 && c_feeMed >= 0 && c_feeHigh >= 0) {
  ui_update_fee_badges_lmh(c_feeLow, c_feeMed, c_feeHigh);
}
  


/*
// ───────────────────────── Footer: 24H Price Card (themed) ─────────────────────────
lv_obj_t* footer = ui::make_card(scr);
lv_obj_set_size(footer, 680, 160);
lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -30);
lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_COLUMN);
lv_obj_set_style_pad_all(footer, 12, 0);
lv_obj_set_style_pad_row(footer, 6, 0);

// Row 1: title
lv_obj_t* footRow = lv_obj_create(footer);
lv_obj_remove_style_all(footRow);
lv_obj_set_size(footRow, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(footRow, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(footRow,
                      LV_FLEX_ALIGN_SPACE_BETWEEN,
                      LV_FLEX_ALIGN_CENTER,
                      LV_FLEX_ALIGN_CENTER);

lv_obj_t* priceChartLabel = lv_label_create(footRow);
lv_obj_add_style(priceChartLabel, &ui::st_title, 0);
lv_obj_set_style_text_font(priceChartLabel, &lv_font_montserrat_14, 0);
lv_label_set_text(priceChartLabel, "24H Bitcoin Price");

// Row 2: chart
priceChart = lv_chart_create(footer);
lv_obj_set_size(priceChart, 640, 90);
lv_obj_set_style_bg_opa(priceChart, LV_OPA_TRANSP, 0);
lv_obj_set_style_border_width(priceChart, 0, 0);
lv_chart_set_type(priceChart, LV_CHART_TYPE_LINE);
lv_chart_set_point_count(priceChart, 24);
lv_chart_set_div_line_count(priceChart, 0, 0);
lv_chart_set_update_mode(priceChart, LV_CHART_UPDATE_MODE_SHIFT);
priceSeries = lv_chart_add_series(priceChart, ui::accent_secondary(), LV_CHART_AXIS_PRIMARY_Y);
for (int i = 0; i < lv_chart_get_point_count(priceChart); i++) priceSeries->y_points[i] = 30000;
lv_chart_set_range(priceChart, LV_CHART_AXIS_PRIMARY_Y, 20000, 80000);
lv_chart_refresh(priceChart);

// Row 3: 24h stat pills (outline-only)
static bool st_pill_outline_inited = false;
static lv_style_t st_pill_outline_teal, st_pill_outline_orange, st_pill_outline_slate;
if (!st_pill_outline_inited) {
  st_pill_outline_inited = true;
  auto init_outline = [](lv_style_t* st, lv_color_t border){
    lv_style_init(st);
    lv_style_set_radius(st, 999);
    lv_style_set_bg_opa(st, LV_OPA_TRANSP);
    lv_style_set_border_width(st, 1);
    lv_style_set_border_color(st, border);
    lv_style_set_text_color(st, lv_color_hex(0xCBD5E1));
    lv_style_set_pad_hor(st, 10);
    lv_style_set_pad_ver(st, 4);
    lv_style_set_text_font(st, &lv_font_montserrat_12);
  };
  init_outline(&st_pill_outline_teal,   ui::accent_secondary());
  init_outline(&st_pill_outline_orange, ui::accent_primary());
  init_outline(&st_pill_outline_slate,  lv_color_hex(0x243142));
}

lv_obj_t* statsRow = lv_obj_create(footer);
lv_obj_remove_style_all(statsRow);
lv_obj_set_size(statsRow, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(statsRow, LV_FLEX_FLOW_ROW);
lv_obj_set_style_pad_gap(statsRow, 8, 0);
lv_obj_set_style_bg_opa(statsRow, LV_OPA_TRANSP, 0);

low24Label = lv_label_create(statsRow);
lv_obj_add_style(low24Label, &st_pill_outline_teal, 0);
lv_label_set_text(low24Label, "Low 24h: —");

high24Label = lv_label_create(statsRow);
lv_obj_add_style(high24Label, &st_pill_outline_orange, 0);
lv_label_set_text(high24Label, "High 24h: —");

vol24Label = lv_label_create(statsRow);
lv_obj_add_style(vol24Label, &st_pill_outline_slate, 0);
lv_label_set_text(vol24Label, "Vol 24h: —");

*/


// ───────────────────────── Footer: Block Interval Visualizer ─────────────────────────
intervalCard = ui::make_card(scr);
lv_obj_set_size(intervalCard, 680, 160);
lv_obj_align(intervalCard, LV_ALIGN_BOTTOM_MID, 0, -30);
lv_obj_set_flex_flow(intervalCard, LV_FLEX_FLOW_COLUMN);
lv_obj_set_style_pad_all(intervalCard, 12, 0);
lv_obj_set_style_pad_row(intervalCard, 6, 0);

// Row 1: title + small pill
lv_obj_t* ivTitle = lv_obj_create(intervalCard);
lv_obj_remove_style_all(ivTitle);
lv_obj_set_size(ivTitle, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(ivTitle, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(ivTitle, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

lv_obj_t* ivLbl = lv_label_create(ivTitle);
lv_obj_add_style(ivLbl, &ui::st_title, 0);
lv_obj_set_style_text_font(ivLbl, &lv_font_montserrat_14, 0);
lv_label_set_text(ivLbl, "Block Interval Visualizer");   // matches dashboard footer title

// outline pill: "target 10min"
lv_obj_t* targetPill = lv_label_create(ivTitle);
lv_obj_set_style_radius(targetPill, 999, 0);
lv_obj_set_style_bg_opa(targetPill, LV_OPA_TRANSP, 0);
lv_obj_set_style_border_width(targetPill, 1, 0);
lv_obj_set_style_border_color(targetPill, lv_color_hex(0x243142), 0);
lv_obj_set_style_text_color(targetPill, lv_color_hex(0xCBD5E1), 0);
lv_obj_set_style_pad_hor(targetPill, 10, 0);
lv_obj_set_style_pad_ver(targetPill, 4, 0);
lv_obj_set_style_text_font(targetPill, &lv_font_montserrat_12, 0);
lv_label_set_text(targetPill, "target 10min");

// Chart: BAR type, 12 columns, 0..60 minutes
intervalChart = lv_chart_create(intervalCard);
lv_obj_set_size(intervalChart, 640, 140);
lv_obj_set_style_bg_opa(intervalChart, LV_OPA_TRANSP, 0);
lv_obj_set_style_border_width(intervalChart, 0, 0);
lv_chart_set_type(intervalChart, LV_CHART_TYPE_BAR);
lv_chart_set_point_count(intervalChart, 12);
lv_chart_set_div_line_count(intervalChart, 0, 0);
lv_chart_set_update_mode(intervalChart, LV_CHART_UPDATE_MODE_SHIFT);
lv_chart_set_range(intervalChart, LV_CHART_AXIS_PRIMARY_Y, 0, 25);

// Outer padding so bars don't touch the card edges
lv_obj_set_style_pad_left(intervalChart, 6, 0);
lv_obj_set_style_pad_right(intervalChart, 6, 0);

// Bar thickness (controls perceived gap between bars)
lv_obj_set_style_size(intervalChart, 10, LV_PART_ITEMS);


// bars use the SECONDARY accent (glowy cyan in your theme)
intervalSeries = lv_chart_add_series(intervalChart, ACC_SECONDARY(), LV_CHART_AXIS_PRIMARY_Y);




// create dashed 10-minute target line over the chart
if (!ivTargetLine) ivTargetLine = lv_line_create(intervalCard);
lv_obj_add_flag(ivTargetLine, LV_OBJ_FLAG_IGNORE_LAYOUT);
lv_obj_set_style_line_width(ivTargetLine, 2, 0);
lv_obj_set_style_line_dash_width(ivTargetLine, 6, 0);
lv_obj_set_style_line_dash_gap(ivTargetLine, 6, 0);
// color complements bars; apply_accent_to_interval() will keep it in sync
lv_obj_set_style_line_color(ivTargetLine, ACC_PRIMARY(), 0);

// position it ~40–50% up, synced to chart range [0..25]
iv_update_target_line();   // call after lv_chart_set_range(...)
// Create the block-number row under the chart
ensure_block_label_row();



// seed demo values 
#if 0
static const uint8_t kDemoIntervals[12] = { 9, 12, 7, 8, 11, 4, 16, 10, 6, 14, 9, 8 };
ui_update_block_intervals(kDemoIntervals, 12);
#endif


// small descriptive line (like your HTML)
lv_obj_t* ivSub = lv_label_create(intervalCard);
lv_obj_set_style_text_color(ivSub, lv_color_hex(0x9AA0A6), 0);
lv_obj_set_style_text_font(ivSub, &lv_font_montserrat_12, 0);
lv_label_set_text(ivSub, "Each column = minutes between blocks; target = 10m.");




  // ...NAVIGATION BUTTONS...

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 25, 20);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_metrics_screen, LV_EVENT_CLICKED, NULL);


  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -25, 20);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_metrics_screen, LV_EVENT_CLICKED, NULL);


// ── Mid-strip (two lines) centered between Back/Right ─────────────
midStrip = lv_obj_create(scr);
lv_obj_remove_style_all(midStrip);
lv_obj_set_size(midStrip, LV_PCT(80), LV_SIZE_CONTENT);

// keep it above the 24h footer
const int FOOTER_H          = 160;
const int FOOTER_BOTTOM_PAD = 30;
const int MIDSTRIP_MARGIN   = 8;
lv_obj_align(midStrip, LV_ALIGN_BOTTOM_MID, 0, -(FOOTER_H + FOOTER_BOTTOM_PAD + MIDSTRIP_MARGIN));

lv_obj_set_flex_flow(midStrip, LV_FLEX_FLOW_COLUMN);
lv_obj_set_style_pad_row(midStrip, 2, 0);
lv_obj_set_style_bg_opa(midStrip, LV_OPA_TRANSP, 0);


// Line 1
midLine1Label = lv_label_create(midStrip);
lv_obj_set_style_text_color(midLine1Label, lv_color_hex(0xCBD5E1), 0); // base slate
lv_obj_set_style_text_font(midLine1Label, &lv_font_montserrat_14, 0);
lv_label_set_long_mode(midLine1Label, LV_LABEL_LONG_SCROLL_CIRCULAR);
lv_label_set_recolor(midLine1Label, true);
{
  char init1[200];
  snprintf(init1, sizeof(init1),
           "Hashrate: #%s — EH/s#  •  Difficulty: #%s —%%# (#%s —d#)",
           g_hex_primary, g_hex_primary, g_hex_primary);
  lv_label_set_text(midLine1Label, init1);
}

// Line 2
midLine2Label = lv_label_create(midStrip);
lv_obj_set_style_text_color(midLine2Label, lv_color_hex(0xCBD5E1), 0); // base slate
lv_obj_set_style_text_font(midLine2Label, &lv_font_montserrat_14, 0);
lv_label_set_long_mode(midLine2Label, LV_LABEL_LONG_SCROLL_CIRCULAR);
lv_label_set_recolor(midLine2Label, true);
{
  char init2[220];
  snprintf(init2, sizeof(init2),
           "Market Cap: #%s —#  •  Circulating Supply: #%s — / 21,000,000#  •  All Time High: #%s —# (#%s —d ago#)",
           g_hex_secondary, g_hex_secondary, g_hex_secondary, g_hex_secondary);
  lv_label_set_text(midLine2Label, init2);
}


// hydrate_metrics_from_cache();

#if 0   // ⛔️ Disable the filler
ui_update_mid_metrics(
  605.0f,   // hashrate EH/s
  -1.23f,   // difficulty change %
  12,       // days since diff adj
  1.23e12f, // market cap
  19780000, // circulating
  69000.0f, // ATH
  650,      // days since ATH
  -12.5f
);
#endif


  apply_accent_swap_to_widgets();
  apply_accent_to_interval(); 

  // ── Timer to refresh middle metrics ──
  static lv_timer_t* mid_timer = nullptr;
  if (!mid_timer) {
    mid_timer = lv_timer_create([](lv_timer_t*) { mid_metrics_tick(); }, 1000, nullptr);
  }

    
  return scr;
}


// --------------------------------------------------
// --------------------------------------------------
// Updates the 24h change pill text + style (green/red)
void ui_update_change_pill(float changePct) {
  c_changePct = changePct;
  c_changePct_set = true;
  if (!changePillLabel) return;

  char sign = (changePct >= 0) ? '+' : '-';
  float absPct = changePct >= 0 ? changePct : -changePct;
  int whole = (int)absPct;
  int frac  = (int)((absPct - whole) * 100 + 0.5f);

  lv_label_set_text_fmt(changePillLabel, "24h: %c%d.%02d%%", sign, whole, frac);
  lv_obj_set_style_bg_color(changePillLabel,
                            (changePct >= 0) ? lv_color_hex(0x22C55E) : lv_color_hex(0xEF4444), 0);
  lv_obj_set_style_text_color(changePillLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(changePillLabel, 999, 0);
  lv_obj_set_style_pad_hor(changePillLabel, 8, 0);
  lv_obj_set_style_pad_ver(changePillLabel, 2, 0);
  lv_obj_set_style_bg_opa(changePillLabel, LV_OPA_COVER, 0);
}

// Compact USD formatter: $12,345 (no cents), K/M/B for large values
static void fmt_usd_compact(float v, char* out, size_t outlen){
  double a = v;
  const char* suffix = "";
  if (a >= 1e9)      { a /= 1e9;  suffix = "B"; }
  else if (a >= 1e6) { a /= 1e6;  suffix = "M"; }
  else if (a >= 1e3) { a /= 1e3;  suffix = "K"; }

  if (*suffix) snprintf(out, outlen, "$%.2f%s", a, suffix);
  else {
    unsigned long whole = (unsigned long)(a + 0.5);
    String s = String(whole);
    for (int i = s.length() - 3; i > 0; i -= 3) s = s.substring(0, i) + "," + s.substring(i);
    snprintf(out, outlen, "$%s", s.c_str());
  }
}

void ui_update_24h_stats(float lowUsd, float highUsd, float volUsd){
  if (low24Label){
    char b[32]; fmt_usd_compact(lowUsd, b, sizeof(b));
    lv_label_set_text_fmt(low24Label,  "Low 24h: %s",  b);
  }
  if (high24Label){
    char b[32]; fmt_usd_compact(highUsd, b, sizeof(b));
    lv_label_set_text_fmt(high24Label, "High 24h: %s", b);
  }
  if (vol24Label){
    char b[32]; fmt_usd_compact(volUsd, b, sizeof(b));
    lv_label_set_text_fmt(vol24Label,  "Vol 24h: %s",  b);
  }
}
