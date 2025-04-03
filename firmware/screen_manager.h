// --- screen_manager.h ---
//BLOKDBIT_Spark v0.002

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

void initMainScreen();
void initMetricsScreen();
void initLifeScreen();
void initBitcoin101Screen();
void initSettingsScreen();

extern lv_obj_t* priceValueLabel;
extern lv_obj_t* blockValueLabel;

#endif
