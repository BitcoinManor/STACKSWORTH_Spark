// SYACKSWORTH_Spark bitaxe_screen.cpp
// SPARKv0.0.3


#include <Arduino.h>
#include "bitaxe_screen.h"
#include "screen_manager.h"
#include "axestack_theme.h"


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
lv_obj_align(lblStacksworth, LV_ALIGN_TOP_LEFT, axe::spacing::md, axe::spacing::sm + 2);

lv_obj_t* lblSpark = lv_label_create(scr);
lv_label_set_text(lblSpark, "// SPARK");
lv_obj_set_style_text_color(lblSpark, lv_color_hex(axe::color::text), 0);
lv_obj_set_style_text_font(lblSpark, &lv_font_montserrat_20, 0);
//lv_obj_align(lblSpark, LV_ALIGN_TOP_LEFT, 190, 10);
lv_obj_align(lblSpark,      LV_ALIGN_TOP_LEFT, axe::spacing::md + 165, axe::spacing::sm + 2);

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


/*
//Chatgpt idea
// RIGHT HEADER: AxeStack \\ Bitaxe Hub (orange + white), with container
lv_obj_t* rightHdr = lv_obj_create(scr);
lv_obj_set_style_bg_opa(rightHdr, LV_OPA_TRANSP, 0);
lv_obj_set_style_border_opa(rightHdr, LV_OPA_TRANSP, 0);
lv_obj_set_style_pad_all(rightHdr, 0, 0);
lv_obj_set_flex_flow(rightHdr, LV_FLEX_FLOW_ROW);
lv_obj_align(rightHdr, LV_ALIGN_TOP_RIGHT, -axe::spacing::lg, axe::spacing::sm + 2);

lv_obj_t* lblAxeStack = lv_label_create(rightHdr);
lv_label_set_text(lblAxeStack, "AxeStack");
lv_obj_set_style_text_color(lblAxeStack, lv_color_hex(axe::brand::orange), 0);
lv_obj_set_style_text_font(lblAxeStack, &lv_font_montserrat_20, 0);

lv_obj_t* lblBitaxe = lv_label_create(rightHdr);
// four backslashes in the source -> shows as two on screen
lv_label_set_text(lblBitaxe, " \\\\ Bitaxe Hub");
lv_obj_set_style_text_color(lblBitaxe, lv_color_hex(axe::color::text), 0);
lv_obj_set_style_text_font(lblBitaxe, &lv_font_montserrat_20, 0);
*/

//New way
// Place the right-most part first (so the whole pair anchors to the right)
lv_obj_t* lblBitaxe = lv_label_create(scr);
lv_label_set_text(lblBitaxe, " \\\\ Bitaxe Hub");   // shows as "\\ Bitaxe Hub"
lv_obj_set_style_text_color(lblBitaxe, lv_color_hex(axe::color::text), 0);
lv_obj_set_style_text_font(lblBitaxe, &lv_font_montserrat_20, 0);
lv_obj_align(lblBitaxe, LV_ALIGN_TOP_RIGHT, -axe::spacing::lg, axe::spacing::sm + 2);

// Now place the left part relative to lblBitaxe (no magic -140)
lv_obj_t* lblAxeStack = lv_label_create(scr);
lv_label_set_text(lblAxeStack, "AxeStack");
lv_obj_set_style_text_color(lblAxeStack, lv_color_hex(axe::brand::orange), 0);
lv_obj_set_style_text_font(lblAxeStack, &lv_font_montserrat_20, 0);

// Position just to the LEFT of lblBitaxe with a small gap
lv_obj_align_to(lblAxeStack, lblBitaxe, LV_ALIGN_OUT_LEFT_MID, -axe::spacing::sm, 0);


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
