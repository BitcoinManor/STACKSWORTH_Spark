// data_store.h
#pragma once
#include <Arduino.h>

namespace Cache {
  struct Price {
    String usdPretty;   // "$69,420.13"
    String cadLine;     // "CAD $94,123.45"
    String satsUsd;     // "1452 SATS / $1 USD"
    String satsCad;     // "1320 SATS / $1 CAD"
    float  changePct = 0.0f;
    bool   changeValid = false;
  };
  struct Fees { int low=-1, med=-1, high=-1; };
  struct Block {
    String height;      // "857,123"
    String miner;       // "Foundry USA"
    uint32_t ts = 0;    // block timestamp (unix) for age calc
  };
  struct Chart24h {
    float points[24];   // USD (footer)
    float mini[24];     // normalized 0..100 (sparkline)
    float low=0, high=0, volUsd=0;
    bool  valid=false;
  };
  struct Weather {
    String time, location, now; // pretty strings you already show
  };

  extern Price price;
  extern Fees fees;
  extern Block block;
  extern Chart24h chart;
  extern Weather weather;

  // helpers
  void setPrice(const String& usdPretty,
                const String& cadLine,
                const String& satsUsd,
                const String& satsCad,
                float changePct, bool changeValid);
  void setFees(int low, int med, int high);
  void setBlock(const String& height, const String& miner, uint32_t ts);
  void setChart(const float* footer24, const float* mini24, float low, float high, float vol);
  void setWeather(const String& time, const String& loc, const String& now);
}
