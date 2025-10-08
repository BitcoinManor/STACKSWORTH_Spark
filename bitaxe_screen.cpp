// SYACKSWORTH_Spark bitaxe_screen.cpp
// SPARKv0.0.3


#include <Arduino.h>
#include "bitaxe_screen.h"
#include "screen_manager.h"
#include "axestack_theme.h"
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


// Handle button press events
void onTouchEvent_bitaxe_screen(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);

  if (target == backBtn) {
    Serial.println("➡️ Back button pressed: Switching to World Screen");
    load_screen(2);
  } else if (target == rightBtn) {
    Serial.println("➡️ Right button pressed: Switching to Settings Screen");
    load_screen(4);
  }
}


// Make a dark card with a thin orange outline (no white bg, slight glow)
static lv_obj_t* make_orange_card(lv_obj_t* parent, const char* title) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_remove_style_all(card);                         // start clean

  // size: half width (2 columns), auto height
  lv_obj_set_size(card, LV_PCT(49), LV_SIZE_CONTENT);

  // dark body
  lv_obj_set_style_bg_color(card, lv_color_hex(0x14181D), 0);
  lv_obj_set_style_bg_opa  (card, LV_OPA_COVER,          0);

  // thin orange border + soft glow
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_shadow_color(card, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_shadow_opa  (card, LV_OPA_20, 0);     // subtle halo
  lv_obj_set_style_shadow_width(card, 8, 0);
  lv_obj_set_style_shadow_spread(card, 0, 0);
  lv_obj_set_style_shadow_ofs_x(card, 0, 0);
  lv_obj_set_style_shadow_ofs_y(card, 0, 0);

  // compact inner spacing and vertical stack
  lv_obj_set_style_pad_all(card, 8, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(card, 4, 0);

  // title
  lv_obj_t* titleLbl = lv_label_create(card);
  lv_label_set_text(titleLbl, title);
  lv_obj_set_style_text_color(titleLbl, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(titleLbl, &lv_font_montserrat_14, 0);

  return card;
}



lv_obj_t* create_bitaxe_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
 
 axe_apply_bg(scr);  // uses tokens.bg

/* 
  // Title label

  lv_obj_t* blokdbitLabel = lv_label_create(scr);
  lv_label_set_text(blokdbitLabel, "STACKSWORTH");
  lv_obj_set_style_text_color(blokdbitLabel, lv_color_hex(axe::color::accent), 0);
  lv_obj_set_style_text_font(blokdbitLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(blokdbitLabel, LV_ALIGN_TOP_LEFT, axe::spacing::md + 9, axe::spacing::sm + 2);

  lv_obj_t* sparkLabel = lv_label_create(scr);
  lv_label_set_text(sparkLabel, "// SPARK");
  lv_obj_set_style_text_color(sparkLabel, lv_color_hex(axe::color::text), 0);
  lv_obj_set_style_text_font(sparkLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(sparkLabel, LV_ALIGN_TOP_LEFT, 190, axe::spacing::sm + 2);

     ??Old Title Label
  lv_obj_t* bitaxeLabel = lv_label_create(scr);
  lv_label_set_text(bitaxeLabel, "Bitaxe Hub");
  lv_obj_set_style_text_color(bitaxeLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(bitaxeLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(bitaxeLabel, LV_ALIGN_TOP_RIGHT, -25, 10);

    //old Footer
  lv_obj_t* inf = lv_label_create(scr);
  lv_label_set_text(inf, "SPARK v0.0.3");
  lv_obj_set_style_text_color(inf, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -60, 0); 
   */

  
  // Header: STACKSWORTH // SPARK (tokenized colors)
lv_obj_t* lblStacksworth = lv_label_create(scr);
lv_label_set_text(lblStacksworth, "STACKSWORTH");
lv_obj_set_style_text_color(lblStacksworth, lv_color_hex(axe::brand::orange), 0);
lv_obj_set_style_text_font(lblStacksworth, &lv_font_montserrat_20, 0);
//lv_obj_align(lblStacksworth, LV_ALIGN_TOP_LEFT, 25, 10);
lv_obj_align(lblStacksworth, LV_ALIGN_TOP_LEFT, axe::spacing::lg, axe::spacing::sm + 2);

lv_obj_t* lblSpark = lv_label_create(scr);
lv_label_set_text(lblSpark, "// SPARK");
lv_obj_set_style_text_color(lblSpark, lv_color_hex(axe::color::text), 0);
lv_obj_set_style_text_font(lblSpark, &lv_font_montserrat_20, 0);
//lv_obj_align(lblSpark, LV_ALIGN_TOP_LEFT, 190, 10);
lv_obj_align(lblSpark, LV_ALIGN_TOP_LEFT, axe::spacing::md + 175, axe::spacing::sm + 2);

/*  //My way that looks good
// Top-right title (tokenized)
lv_obj_t* lblAxeStack = lv_label_create(scr);
lv_label_set_text(lblAxeStack, "AxeStack");
lv_obj_set_style_text_color(lblAxeStack, lv_color_hex(axe::brand::orange), 0);  
lv_obj_set_style_text_font(lblAxeStack, &lv_font_montserrat_20, 0);             
lv_obj_align(lblAxeStack, LV_ALIGN_TOP_RIGHT, -140, 10);


lv_obj_t* lblBitaxe = lv_label_create(scr);
lv_label_set_text(lblBitaxe, " \\\\ Bitaxe Hub");
lv_obj_set_style_text_color(lblBitaxe, lv_color_hex(axe::color::text), 0);  
lv_obj_set_style_text_font(lblBitaxe, &lv_font_montserrat_20, 0);             
lv_obj_align(lblBitaxe, LV_ALIGN_TOP_RIGHT, -25, 10);
*/



//New way using axestack_tokens
// Place the right-most part first (so the whole pair anchors to the right)
lv_obj_t* lblBitaxe = lv_label_create(scr);
lv_label_set_text(lblBitaxe, " \\\\ Bitaxe Hub");   // shows as "\\ Bitaxe Hub"
lv_obj_set_style_text_color(lblBitaxe, lv_color_hex(axe::color::text), 0);
lv_obj_set_style_text_font(lblBitaxe, &lv_font_montserrat_20, 0);
lv_obj_align(lblBitaxe, LV_ALIGN_TOP_RIGHT, -axe::spacing::xl, axe::spacing::sm + 2);

// Now place the left part relative to lblBitaxe (no magic -140)
lv_obj_t* lblAxeStack = lv_label_create(scr);
lv_label_set_text(lblAxeStack, "AxeStack");
lv_obj_set_style_text_color(lblAxeStack, lv_color_hex(axe::brand::orange), 0);
lv_obj_set_style_text_font(lblAxeStack, &lv_font_montserrat_20, 0);

// Position just to the LEFT of lblBitaxe with a small gap
lv_obj_align_to(lblAxeStack, lblBitaxe, LV_ALIGN_OUT_LEFT_MID, -axe::spacing::xs - 2, 0);


 // ── ROW 1: two cards under the titles
lv_obj_t* row1 = lv_obj_create(scr);
lv_obj_remove_style_all(row1);
lv_obj_set_style_bg_opa(row1, LV_OPA_TRANSP, 0);
lv_obj_set_size(row1, LV_PCT(100), LV_SIZE_CONTENT);

// place it under your title labels; adjust Y (44) to taste
lv_obj_align(row1, LV_ALIGN_TOP_MID, 0, 44);

// make it a tidy horizontal row (no wrap), small gutters
lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
lv_obj_set_style_pad_left  (row1, axe::spacing::lg, 0);
lv_obj_set_style_pad_right (row1, axe::spacing::lg, 0);
lv_obj_set_style_pad_gap   (row1, 8, 0);

// your standard card factory — consistent look with other screens
lv_obj_t* cardTemp = ui::make_card(row1);
lv_obj_set_size(cardTemp, LV_PCT(49), LV_SIZE_CONTENT);

lv_obj_t* cardFan  = ui::make_card(row1);
lv_obj_set_size(cardFan,  LV_PCT(49), LV_SIZE_CONTENT);

// minimal titles only (we’ll style/border later)
lv_obj_t* t1 = lv_label_create(cardTemp);
lv_label_set_text(t1, "Temperature");

lv_obj_t* t2 = lv_label_create(cardFan);
lv_label_set_text(t2, "Fan");







    // Footer info (keeps your text, switches to tokens)
lv_obj_t* inf = axe_make_label(scr, "SPARK v0.0.3", axe::type::body, axe::color::text);
lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
    //lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -60, 0);
lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -(axe::spacing::lg + 36), -axe::spacing::xs);

 
 


  //...BUTTONS...

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 25, 20);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_bitaxe_screen, LV_EVENT_CLICKED, NULL);

  
  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_RIGHT_MID, -25, 20);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_bitaxe_screen, LV_EVENT_CLICKED, NULL);
  

  return scr;
}
