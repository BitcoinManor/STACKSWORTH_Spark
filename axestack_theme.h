#pragma once
#include <lvgl.h>
#include "axestack_tokens.h"

inline void axe_apply_bg(lv_obj_t* root) {
  lv_obj_set_style_bg_color(root, lv_color_hex(axe::color::bg), 0);
}

inline lv_obj_t* axe_make_label(lv_obj_t* parent, const char* text, int size_px, uint32_t color_hex) {
  lv_obj_t* lbl = lv_label_create(parent);
  lv_label_set_text(lbl, text);
  lv_obj_set_style_text_color(lbl, lv_color_hex(color_hex), 0);
  lv_obj_set_style_text_font(lbl, LV_FONT_DEFAULT, 0); // temp: default font
  (void)size_px; // placeholder until we wire proper fonts
  return lbl;
}
