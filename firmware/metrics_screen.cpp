#include <lvgl.h>
#include "metrics_screen.h"
#include "screen_manager.h"  // For shared styles

void initMetricsScreen() {
  lv_obj_t* metricsPanel = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(metricsPanel);
  lv_obj_set_size(metricsPanel, 760, 440);
  lv_obj_align(metricsPanel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_opa(metricsPanel, LV_OPA_TRANSP, 0);
  lv_obj_add_style(metricsPanel, &widgetStyle, 0);
  lv_obj_add_style(metricsPanel, &glowStyle, 0);

  lv_obj_t* titleLabel = lv_label_create(metricsPanel);
  lv_label_set_text(titleLabel, "BITCOIN METRICS");
  lv_obj_set_style_text_color(titleLabel, lv_color_hex(0xFCA420), 0);
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_18, 0);
  lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t* widget1 = lv_obj_create(metricsPanel);
  lv_obj_set_size(widget1, 320, 120);
  lv_obj_align(widget1, LV_ALIGN_LEFT_MID, 20, 0);
  lv_obj_add_style(widget1, &widget2Style, 0);
  lv_obj_add_style(widget1, &glowStyle, 0);
  lv_label_set_text(lv_label_create(widget1), "Mempool Size");

  lv_obj_t* widget2 = lv_obj_create(metricsPanel);
  lv_obj_set_size(widget2, 320, 120);
  lv_obj_align(widget2, LV_ALIGN_RIGHT_MID, -20, 0);
  lv_obj_add_style(widget2, &widget2Style, 0);
  lv_obj_add_style(widget2, &glowStyle, 0);
  lv_label_set_text(lv_label_create(widget2), "Network Hashrate");
}
