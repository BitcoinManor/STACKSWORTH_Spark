// BLOKDBIT_Spark screen_manager.cpp
// SPARKv0.03 – Screen switching with swipe + arrow

#include <lvgl.h>
#include "screen_manager.h"
#include "metrics_screen.h"
#include "screen_b.h"

void screen_manager_init() {
  lv_scr_load(create_metrics_screen());
}

void load_screen(int index) {
  if (index == 0) {
    lv_scr_load(create_metrics_screen());
  } else if (index == 1) {
    lv_scr_load(create_screen_b());
  }
}
