// STACKSWORTH_Spark settings_screen.cpp
// SPARKv0.0.3


#include <Arduino.h>
#include "settings_screen.h"
#include "screen_manager.h"
#include "world_screen.h"
#include <Preferences.h> 
#include "ui_theme.h"

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



static lv_obj_t* g_clockSwitch = nullptr;

static void clock_switch_cb(lv_event_t* e) {
  lv_obj_t* sw = lv_event_get_target(e);
  if (!sw) return;
  bool use12h = lv_obj_has_state(sw, LV_STATE_CHECKED);

  // Persist
  Preferences prefs;
  prefs.begin("cfg", false);
  prefs.putBool("clock12h", use12h);
  prefs.end();

  // Apply immediately (updates world_screen time label)
  ui_weather_set_clock_12h(use12h);
}

// Add a "Clock format (12h)" row with a switch into the given parent container.
static void add_clock_format_setting(lv_obj_t* parent) {
  if (!parent) parent = lv_scr_act();

  // Read current pref (default true → 12h)
  Preferences prefs;
  prefs.begin("cfg", true);
  bool use12h = prefs.getBool("clock12h", true);
  prefs.end();

  // Row container
  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row,
                        LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_hor(row, 8, 0);
  lv_obj_set_style_pad_ver(row, 4, 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

  // Label
  lv_obj_t* lbl = lv_label_create(row);
  lv_label_set_text(lbl, "Clock format (12h)");

  // Switch
  g_clockSwitch = lv_switch_create(row);
  if (use12h) lv_obj_add_state(g_clockSwitch, LV_STATE_CHECKED);
  lv_obj_add_event_cb(g_clockSwitch, clock_switch_cb, LV_EVENT_VALUE_CHANGED, nullptr);
}


// ----- Currency setting (2nd-line fiat for CAD/SATS/$1) -----
static lv_obj_t* g_currencyDrop = nullptr;

static int currency_index_from_iso(const String& iso) {
  if (iso.equalsIgnoreCase("CAD")) return 0;
  if (iso.equalsIgnoreCase("USD")) return 1;
  if (iso.equalsIgnoreCase("EUR")) return 2;
  if (iso.equalsIgnoreCase("GBP")) return 3;
  if (iso.equalsIgnoreCase("JPY")) return 4;
  if (iso.equalsIgnoreCase("AUD")) return 5;
  return 0; // default CAD
}

static String currency_iso_from_index(int idx) {
  switch (idx) {
    case 0: return "CAD";
    case 1: return "USD";
    case 2: return "EUR";
    case 3: return "GBP";
    case 4: return "JPY";
    case 5: return "AUD";
    default: return "CAD";
  }
}

static void currency_drop_cb(lv_event_t* e) {
  lv_obj_t* dd = lv_event_get_target(e);
  uint16_t sel = lv_dropdown_get_selected(dd);
  String iso = currency_iso_from_index((int)sel);

  // Persist preference
  Preferences prefs;
  prefs.begin("cfg", false);
  prefs.putString("fiat", iso);
  prefs.end();

  // Tell the metrics screen to repaint its “CAD line” + “SATS / 1 <fiat>” line
  extern void ui_price_set_preferred_fiat(const String& iso);  // declared in metrics_screen.h
  ui_price_set_preferred_fiat(iso);
}

static void add_currency_setting(lv_obj_t* parent) {
  if (!parent) parent = lv_scr_act();

  // Load current
  Preferences prefs;
  prefs.begin("cfg", true);
  String iso = prefs.getString("fiat", "CAD");
  prefs.end();

  // Row
  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row,
                        LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_hor(row, 8, 0);
  lv_obj_set_style_pad_ver(row, 4, 0);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

  // Label
  lv_obj_t* lbl = lv_label_create(row);
  lv_label_set_text(lbl, "Second currency");

  // Dropdown
  g_currencyDrop = lv_dropdown_create(row);
  // Options order matches currency_index_from_iso()
  lv_dropdown_set_options_static(g_currencyDrop, "CAD\nUSD\nEUR\nGBP\nJPY\nAUD");
  lv_dropdown_set_selected(g_currencyDrop, currency_index_from_iso(iso));
  lv_obj_add_event_cb(g_currencyDrop, currency_drop_cb, LV_EVENT_VALUE_CHANGED, nullptr);
}



// Handle button press events
void onTouchEvent_settings_screen(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == backBtn) {
    //isLeftOn = !isLeftOn;
    Serial.println("➡️ Back button pressed: Switching to Metrics Screen");
    load_screen(3);
  } else if (target == rightBtn) {
    Serial.println("➡️ Right button pressed: Switching to new screen");
    load_screen(0);
  }
}



 



//***Create Screen

lv_obj_t* create_settings_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); //Black
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0); 




  // Title labels

  lv_obj_t* blokdbitLabel = lv_label_create(scr);
  lv_label_set_text(blokdbitLabel, "STACKSWORTH");
  lv_obj_set_style_text_color(blokdbitLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(blokdbitLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(blokdbitLabel, LV_ALIGN_TOP_LEFT, 25, 10);

  lv_obj_t* sparkLabel = lv_label_create(scr);
  lv_label_set_text(sparkLabel, "SPARK");
  lv_obj_set_style_text_color(sparkLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(sparkLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(sparkLabel, LV_ALIGN_TOP_LEFT, 190, 35);

  lv_obj_t* priceheightLabel = lv_label_create(scr);
  lv_label_set_text(priceheightLabel, "SETTINGS");
  lv_obj_set_style_text_color(priceheightLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(priceheightLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(priceheightLabel, LV_ALIGN_TOP_RIGHT, -25, 10);


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

// ========== Row 1: Title  ==========
lv_obj_t* row1 = lv_obj_create(widget1);
lv_obj_remove_style_all(row1);
lv_obj_set_size(row1, LV_PCT(100), LV_SIZE_CONTENT);
lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(row1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
lv_obj_set_style_bg_opa(row1, LV_OPA_TRANSP, 0);

// Title 
lv_obj_t* clockFormatTitle = lv_label_create(row1);
lv_obj_add_style(clockFormatTitle, &ui::st_title, 0);
lv_label_set_text(clockFormatTitle, "Clock Format");
lv_obj_set_style_text_font(clockFormatTitle, &lv_font_montserrat_16, 0);


  // Section title inside the card
lv_obj_t* prefsTitle = lv_label_create(widget1);
lv_obj_add_style(prefsTitle, &ui::st_title, 0);
lv_obj_set_style_text_font(prefsTitle, &lv_font_montserrat_16, 0);
lv_label_set_text(prefsTitle, "Preferences");

// Add settings rows into the card (not on the root)
add_clock_format_setting(widget1);
add_currency_setting(widget1);



  //...BUTTONS...

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 25, 20);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_settings_screen, LV_EVENT_CLICKED, NULL);


  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -25, 20);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_settings_screen, LV_EVENT_CLICKED, NULL);
  

  return scr;
}
