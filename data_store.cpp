// data_store.cpp
#include "data_store.h"

namespace Cache {
  Price    price;
  Fees     fees;
  Block    block;
  Chart24h chart;
  Weather  weather;

  void setPrice(const String& u, const String& c,
                const String& su, const String& sc,
                float pct, bool ok) {
    price.usdPretty = u;
    price.cadLine   = c;
    price.satsUsd   = su;
    price.satsCad   = sc;
    price.changePct = pct;
    price.changeValid = ok;
  }

  void setFees(int low, int med, int high) {
    fees.low = low; fees.med = med; fees.high = high;
  }

  void setBlock(const String& h, const String& m, uint32_t ts_) {
    block.height = h;
    block.miner  = m;
    block.ts     = ts_;
  }

  void setChart(const float* footer24, const float* mini24,
                float lowUsd, float highUsd, float volUsd) {
    for (int i=0;i<24;i++) {
      chart.points[i] = footer24 ? footer24[i] : 0.0f;
      chart.mini[i]   = mini24   ? mini24[i]   : 0.0f;
    }
    chart.low   = lowUsd;
    chart.high  = highUsd;
    chart.volUsd= volUsd;
    chart.valid = true;
  }

  void setWeather(const String& t, const String& loc, const String& now) {
    weather.time = t;
    weather.location = loc;
    weather.now = now;
  }
}
