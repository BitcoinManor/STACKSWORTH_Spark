// BLOKDBIT_Spark screen_manager.h
// SPARKv0.03

#pragma once
#include <lvgl.h>

// 🔁 Shared button objects (created on every screen)
extern lv_obj_t* backBtn;
extern lv_obj_t* rightBtn;

// 🧠 Optional: Shared toggle state (if you're still using these)
extern bool isLeftOn;
extern bool isRightOn;

// 📈 Labels updated by main loop fetches
extern lv_obj_t* priceValueLabel;
extern lv_obj_t* blockValueLabel;
extern lv_obj_t* feeValueLabel;
extern lv_obj_t* solvedByValueLabel;

// 🚀 Screen control functions
void screen_manager_init();
void load_screen(int index);
