// BLOKDBIT_Spark screen_manager.cpp
// SPARKv1.0.0 – Screen switching with swipe + arrow

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
  // Clean up the current screen to prevent memory leaks
  lv_obj_t* current_scr = lv_scr_act();
  
  // Create new screen
  lv_obj_t* new_scr = nullptr;
  if (index == 0) {
    new_scr = create_metrics_screen();
  } else if (index == 1) {
    new_scr = create_bigstats_screen();
  } else if (index == 2) {
    new_scr = create_world_screen();
  } else if (index == 3) {
    new_scr = create_bitaxe_screen();
  } else if (index == 4) {
    new_scr = create_settings_screen();
  }
  
  if (new_scr) {
    // Load new screen first
    lv_scr_load(new_scr);
    
    // Clean up old screen after successful load
    if (current_scr && current_scr != new_scr) {
      lv_obj_del_async(current_scr);
    }
  }
}
