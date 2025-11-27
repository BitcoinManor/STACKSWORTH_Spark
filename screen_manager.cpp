// BLOKDBIT_Spark screen_manager.cpp
// SPARKv1.0.0 – Screen switching with swipe + arrow + memory cleanup

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

// Current screen tracking for swipe navigation
static int current_screen_index = 0;
static const int TOTAL_SCREENS = 5;

// Swipe gesture handler
void swipe_event_handler(lv_event_t* e) {
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  
  if (dir == LV_DIR_LEFT) {
    // Swipe left -> next screen
    int next_screen = (current_screen_index + 1) % TOTAL_SCREENS;
    Serial.print("👈 Swipe left detected: Moving to screen ");
    Serial.println(next_screen);
    load_screen(next_screen);
  } 
  else if (dir == LV_DIR_RIGHT) {
    // Swipe right -> previous screen  
    int prev_screen = (current_screen_index - 1 + TOTAL_SCREENS) % TOTAL_SCREENS;
    Serial.print("👉 Swipe right detected: Moving to screen ");
    Serial.println(prev_screen);
    load_screen(prev_screen);
  }
}

void screen_manager_init() {
  load_screen(0); // Use load_screen to properly set up swipe gestures
}

void load_screen(int index) {
  // Update current screen tracking
  current_screen_index = index;
  
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
    // Enable swipe gestures on the new screen
    lv_obj_add_event_cb(new_scr, swipe_event_handler, LV_EVENT_GESTURE, NULL);
    lv_obj_clear_flag(new_scr, LV_OBJ_FLAG_GESTURE_BUBBLE);
    
    // Load new screen first
    lv_scr_load(new_scr);
    
    // Clean up old screen after successful load
    if (current_scr && current_scr != new_scr) {
      lv_obj_del_async(current_scr);
    }
  }
}
