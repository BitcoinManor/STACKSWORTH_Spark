// BLOKDBIT_Spark screen_manager.cpp
// SPARKv0.03 – Screen switching with swipe + arrow

#include <lvgl.h>
#include "screen_manager.h"
#include "metrics_screen.h"
#include "bigstats_screen.h"
#include "world_screen.h"
#include "bitaxe_screen.h"
#include "settings_screen.h"


// screen_manager.cpp
lv_obj_t* backBtn = nullptr;
lv_obj_t* rightBtn = nullptr;
bool isLeftOn = false;
bool isRightOn = false;


void screen_manager_init() {
  lv_scr_load(create_metrics_screen());
}

void load_screen(int index) {
  if (index == 0) {
    lv_scr_load(create_metrics_screen());
  } else if (index == 1) {
    lv_scr_load(create_bigstats_screen());
  } else if (index == 2) {
    lv_scr_load(create_world_screen());
  } else if (index == 3) {
    lv_scr_load(create_bitaxe_screen());
  } else if (index == 4) {
    lv_scr_load(create_settings_screen());
  }
}
