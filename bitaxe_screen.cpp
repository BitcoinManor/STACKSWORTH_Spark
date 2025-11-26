// SYACKSWORTH_Spark bitaxe_screen.cpp
// SPARKv1.0.0


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

  // Header: STACKSWORTH // SPARK (tokenized colors)
  lv_obj_t* lblStacksworth = lv_label_create(scr);
  lv_label_set_text(lblStacksworth, "STACKSWORTH");
  lv_obj_set_style_text_color(lblStacksworth, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(lblStacksworth, &lv_font_montserrat_20, 0);
  lv_obj_align(lblStacksworth, LV_ALIGN_TOP_LEFT, axe::spacing::lg, axe::spacing::sm + 2);

  lv_obj_t* lblSpark = lv_label_create(scr);
  lv_label_set_text(lblSpark, "// SPARK");
  lv_obj_set_style_text_color(lblSpark, lv_color_hex(axe::color::text), 0);
  lv_obj_set_style_text_font(lblSpark, &lv_font_montserrat_20, 0);
  lv_obj_align(lblSpark, LV_ALIGN_TOP_LEFT, axe::spacing::md + 175, axe::spacing::sm + 2);

  // Top-right title: AxeStack \\ Bitaxe Hub
  lv_obj_t* lblBitaxe = lv_label_create(scr);
  lv_label_set_text(lblBitaxe, " \\\\ Bitaxe Hub");   // shows as "\\ Bitaxe Hub"
  lv_obj_set_style_text_color(lblBitaxe, lv_color_hex(axe::color::text), 0);
  lv_obj_set_style_text_font(lblBitaxe, &lv_font_montserrat_20, 0);
  lv_obj_align(lblBitaxe, LV_ALIGN_TOP_RIGHT, -axe::spacing::xl, axe::spacing::sm + 2);

  lv_obj_t* lblAxeStack = lv_label_create(scr);
  lv_label_set_text(lblAxeStack, "AxeStack");
  lv_obj_set_style_text_color(lblAxeStack, lv_color_hex(axe::brand::orange), 0);
  lv_obj_set_style_text_font(lblAxeStack, &lv_font_montserrat_20, 0);
  lv_obj_align_to(lblAxeStack, lblBitaxe, LV_ALIGN_OUT_LEFT_MID, -axe::spacing::xs - 2, 0);

  // ============================================================
  //   BITAXE WIDGET CLUSTER (190mm x 140mm → scaled panel)
  //   8 empty cards laid out exactly like your blueprint
  // ============================================================

  // Wider layout for better 7" screen utilization, keeping height manageable
  // due to title/version space constraints at top and bottom.
  const int FRAME_INNER_W = 700;   // wider for better screen usage  
  const int FRAME_INNER_H = 400;   // keep original height (title/version limits)
  const int FRAME_PAD     = 12;    // inner margin between border and widgets

  const int FRAME_W = FRAME_INNER_W + FRAME_PAD * 2;
  const int FRAME_H = FRAME_INNER_H + FRAME_PAD * 2;

  // Outer framed container with soft neon cyan glow
  lv_obj_t* frame = lv_obj_create(scr);
  lv_obj_remove_style_all(frame);
  lv_obj_set_size(frame, FRAME_W, FRAME_H);

  // Dark background for the panel
  lv_obj_set_style_bg_color(frame, lv_color_hex(0x05070A), 0);
  lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);

  // Neon cyan border + soft glow
  lv_obj_set_style_border_width(frame, 2, 0);
  lv_obj_set_style_border_color(frame, lv_color_hex(0x00F0FF), 0);
  lv_obj_set_style_radius(frame, 12, 0);

  lv_obj_set_style_shadow_color(frame, lv_color_hex(0x00F0FF), 0);
  lv_obj_set_style_shadow_opa(frame, LV_OPA_30, 0);
  lv_obj_set_style_shadow_width(frame, 18, 0);
  lv_obj_set_style_shadow_spread(frame, 0, 0);
  lv_obj_set_style_shadow_ofs_x(frame, 0, 0);
  lv_obj_set_style_shadow_ofs_y(frame, 0, 0);

  // Position the panel towards the bottom, leaving room for headers and footer
  lv_obj_align(frame, LV_ALIGN_BOTTOM_MID, 0, -axe::spacing::lg);

  // Helper to create an empty widget card (no labels/icons)
  auto make_empty_widget = [](lv_obj_t* parent) -> lv_obj_t* {
    lv_obj_t* card = ui::make_card(parent);  // keeps global card style
    // No labels or content – pure empty widget
    return card;
  };

  // NOTE: All positions below are scaled from your mm layout
  // using a factor of 400px / 140mm. These ints are precomputed.

  // Widget 1: 40mm x 40mm at (0, 0)
  lv_obj_t* w1 = make_empty_widget(frame);
  lv_obj_set_size(w1, 147, 114);
  lv_obj_set_pos (w1, FRAME_PAD + 0, FRAME_PAD + 0);

  // Widget 2: 85mm x 40mm at (45mm, 0)
  lv_obj_t* w2 = make_empty_widget(frame);
  lv_obj_set_size(w2, 313, 114);
  lv_obj_set_pos (w2,
                  FRAME_PAD + 129,  // x
                  FRAME_PAD + 0);   // y

  // Widget 3: 55mm x 75mm at (135mm, 0)
  lv_obj_t* w3 = make_empty_widget(frame);
  lv_obj_set_size(w3, 202, 214);
  lv_obj_set_pos (w3,
                  FRAME_PAD + 386,
                  FRAME_PAD + 0);

  // Widget 4: 40mm x 40mm at (0, 45mm)
  lv_obj_t* w4 = make_empty_widget(frame);
  lv_obj_set_size(w4, 147, 114);
  lv_obj_set_pos (w4,
                  FRAME_PAD + 0,
                  FRAME_PAD + 129);

  // Widget 5: 40mm x 40mm at (45mm, 45mm)
  lv_obj_t* w5 = make_empty_widget(frame);
  lv_obj_set_size(w5, 147, 114);
  lv_obj_set_pos (w5,
                  FRAME_PAD + 129,
                  FRAME_PAD + 129);

  // Widget 6: 40mm x 95mm at (90mm, 45mm)
  lv_obj_t* w6 = make_empty_widget(frame);
  lv_obj_set_size(w6, 147, 271);
  lv_obj_set_pos (w6,
                  FRAME_PAD + 257,
                  FRAME_PAD + 129);

  // Widget 7: 85mm x 50mm at (0, 90mm)
  lv_obj_t* w7 = make_empty_widget(frame);
  lv_obj_set_size(w7, 313, 143);
  lv_obj_set_pos (w7,
                  FRAME_PAD + 0,
                  FRAME_PAD + 257);

  // Widget 8: 55mm x 60mm at (135mm, 80mm)
  lv_obj_t* w8 = make_empty_widget(frame);
  lv_obj_set_size(w8, 202, 171);
  lv_obj_set_pos (w8,
                  FRAME_PAD + 386,
                  FRAME_PAD + 229);

  // ============================================================
  // Footer info (keeps your text, switches to tokens)
  // ============================================================
  lv_obj_t* inf = axe_make_label(scr, "SPARK v0.0.3", axe::type::body, axe::color::text);
  lv_obj_set_style_text_font(inf, &lv_font_montserrat_16, 0);
  lv_obj_align(inf, LV_ALIGN_BOTTOM_RIGHT, -(axe::spacing::lg + 36), -axe::spacing::xs);

  // ============================================================
  // Navigation BUTTONS
  // ============================================================

  // Back Button
  backBtn = lv_obj_create(scr);
  lv_obj_set_size(backBtn, 30, 30);
  lv_obj_align(backBtn, LV_ALIGN_BOTTOM_LEFT, 25, -60);
  lv_obj_add_style(backBtn, &orangeStyle, 0);
  lv_obj_add_event_cb(backBtn, onTouchEvent_bitaxe_screen, LV_EVENT_CLICKED, NULL);

  // Right Button
  rightBtn = lv_obj_create(scr);
  lv_obj_set_size(rightBtn, 30, 30);
  lv_obj_align(rightBtn, LV_ALIGN_BOTTOM_LEFT, 70, -60);
  lv_obj_add_style(rightBtn, &blueStyle, 0);
  lv_obj_add_event_cb(rightBtn, onTouchEvent_bitaxe_screen, LV_EVENT_CLICKED, NULL);

  return scr;
}
