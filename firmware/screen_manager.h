
// BLOKDBIT_Spark screen_manager.h
// SPARKv0.03


#pragma once
#include <lvgl.h>


// External labels that other screens update
extern lv_obj_t* priceValueLabel;
extern lv_obj_t* blockValueLabel;

void screen_manager_init();
void load_screen(int index);
