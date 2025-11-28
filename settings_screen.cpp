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
static bool screen_switching = false;  // Prevent recursive calls
static lv_obj_t* current_screen_obj = nullptr;  // Track current screen object

// Swipe gesture handler
void swipe_event_handler(lv_event_t* e) {
  if (screen_switching) {
    Serial.println("⚠️ Screen switch already in progress, ignoring swipe");
    return;  // Prevent recursive calls
  }
  
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
  Serial.println("🚀 Initializing screen manager...");
  screen_switching = false;
  current_screen_obj = nullptr;
  load_screen(0); // Use load_screen to properly set up swipe gestures
}

void load_screen(int index) {
  if (screen_switching) {
    Serial.println("⚠️ Screen switch already in progress, ignoring request");
    return;  // Prevent recursive calls
  }
  
  Serial.print("🔄 Loading screen ");
  Serial.println(index);
  
  screen_switching = true;
  
  // Update current screen tracking
  current_screen_index = index;
  
  // Get current screen - but don't rely on lv_scr_act() during transitions
  lv_obj_t* old_scr = current_screen_obj;
  
  // Create new screen with error checking
  lv_obj_t* new_scr = nullptr;
  try {
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
  } catch (...) {
    Serial.println("❌ Error creating screen!");
    screen_switching = false;
    return;
  }
  
  if (!new_scr) {
    Serial.println("❌ Failed to create new screen!");
    screen_switching = false;
    return;
  }
  
  Serial.println("✅ New screen created successfully");
  
  // Load new screen first - this is critical
  lv_scr_load(new_scr);
  current_screen_obj = new_scr;
  
  // Small delay to let screen load
  delay(10);
  
  // Enable swipe gestures on the new screen
  lv_obj_add_event_cb(new_scr, swipe_event_handler, LV_EVENT_GESTURE, NULL);
  lv_obj_clear_flag(new_scr, LV_OBJ_FLAG_GESTURE_BUBBLE);
  
  // Clean up old screen safely - only if it's different and valid
  if (old_scr && old_scr != new_scr && lv_obj_is_valid(old_scr)) {
    Serial.println("🗑️ Cleaning up old screen");
    lv_obj_del_async(old_scr);
  }
  
  Serial.println("✅ Screen transition complete");
  screen_switching = false;  // Reset flag
}
