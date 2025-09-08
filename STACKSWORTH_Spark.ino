/***************************************************************************************
 *  STACKSWORTH Spark – "mainScreen" UI
 *  --------------------------------------------------
 *  Project     : STACKSWORTH Spark Firmware
 *  Version     : v0.0.2
 *  Device      : ESP32-S3 Waveshare 7" Touchscreen (800x480)
 *  Description : Modular Bitcoin Dashboard UI using LVGL
 *  Designer    : Bitcoin Manor 🟧
 * 
 *  
 *  💡 Easter Egg: Try tapping the infinity label in v0.1 😉
 ***************************************************************************************/




 #include <Arduino.h>
 #include <ArduinoJson.h>
 #include <WiFi.h>
 #include <HTTPClient.h>
 #include <Waveshare_ST7262_LVGL.h>
 #include <lvgl.h>
 #include "metrics_screen.h"
 #include "bigstats_screen.h"
 #include "world_screen.h"
 #include "bitaxe_screen.h"
 #include "settings_screen.h"
 #include "screen_manager.h"
 #include "ui_theme.h"
 #include <float.h>

 // ---- pretty price formatting: $12,345.67 ----
static String fmt_with_commas(unsigned long v) {
  String s = String(v);
  int insert = s.length() - 3;
  while (insert > 0) {
    s = s.substring(0, insert) + "," + s.substring(insert);
    insert -= 3;
  }
  return s;
}

static void format_price_usd(float value, char* out, size_t outlen, const char* prefix="$") {
  if (value < 0) value = 0;
  unsigned long whole = (unsigned long)value;
  int cents = (int)((value - (float)whole) * 100.0f + 0.5f);
  if (cents >= 100) { whole++; cents -= 100; }
  String s = String(prefix) + fmt_with_commas(whole) + "." + (cents < 10 ? "0" : "") + String(cents);
  s.toCharArray(out, outlen);
}


// Define shared global variables
//String lastPrice = "Loading...";
//String lastFee = "Loading...";
//String lastBlockHeight = "Loading...";
//String lastMiner = "Loading...";
 
 
 // Button pointers and toggle flags
//lv_obj_t* backBtn;
//lv_obj_t* rightBtn;
//bool isLeftOn = false;
//bool isRightOn = false;

 
 // Styles
 String title = "STACKSWORTH Spark – Satscape v0.0.2";
 lv_style_t widgetStyle;
 lv_style_t widget2Style;
 lv_style_t glowStyle;
 
 lv_style_t orangeStyle, greenStyle;
 lv_style_t blueStyle, yellowStyle;
 
 


 
 struct MinerTag {
   const char* tag;
   const char* name;
 };
 
 const MinerTag knownTags[] = {
   { "f2pool", "F2Pool" },
   { "antpool", "AntPool" },
   { "viabtc", "ViaBTC" },
   { "poolin", "Poolin" },
   { "btccom", "BTC.com" },
   { "binance", "Binance Pool" },
   { "carbon", "Carbon Negative" },
   { "slush", "Slush Pool" },
   { "braiins", "Braiins Pool" },
   { "foundry", "Foundry USA" },
   { "ocean", "Ocean Pool" },
   { "mara", "Marathon" },
   { "marathon", "Marathon" },
   { "luxor", "Luxor" },
   { "ultimus", "ULTIMUSPOOL" },
   { "novablock", "NovaBlock" },
   { "sigma", "SigmaPool" },
   { "spider", "SpiderPool" },
   { "tera", "TERA Pool" },
   { "okex", "OKEx Pool" },
   { "kucoin", "KuCoin Pool" },
   { "sbi", "SBI Crypto" },
   { "btctop", "BTC.TOP" },
   { "emcd", "EMCD Pool" },
   { "secpool", "SECPOOL" },
   { "hz", "HZ Pool" },
   { "solo.ckpool", "Solo CKPool" },
   { "solopool", "Solo Pool" },
   { "solo", "Solo Miner" },           // Catch-all fallback for anything solo-ish
   { "bitaxe", "Bitaxe Solo Miner" },
   { "node.pw", "Node.PW" },
   { "/axe/", "Bitaxe Solo Miner" }            
 };
 
 String hexToAscii(const String& hex) {
   String ascii = "";
   for (unsigned int i = 0; i < hex.length(); i += 2) {
     String byteString = hex.substring(i, i + 2);
     char c = (char) strtol(byteString.c_str(), nullptr, 16);
     if (isPrintable(c)) {
       ascii += c;
     }
   }
   return ascii;
 }
 
 
 String identifyMiner(String scriptSig) {
   scriptSig.toLowerCase(); // Normalize for comparison
 
   for (const auto& tag : knownTags) {
     if (scriptSig.indexOf(tag.tag) != -1) {
       return tag.name;
     }
   }
 
   // Optional: return partial string for unknowns (dev mode)
   if (scriptSig.length() > 0) {
     int maxLen = 24;
     String preview = scriptSig.substring(0, min((int)scriptSig.length(), maxLen));
     return "Unknown (" + preview + ")";
   }
 
   return "Unknown";
 }
 
 
 
 // Wi-Fi Credentials
 const char* ssid = "SM-S918W0853";
 const char* password = "MySamsungPhone!!!";
 //const char* ssid = "[ in-juh-noo-i-tee ]";
 //const char* password = "notachance";
 
 //int lastBlockHeight = 0;

 // 🌐 Cached values for display labels (prevent flicker / uploading state)
 String lastPrice = "…";            // Show loading dots initially
 String lastFee = "…";
 String lastBlockHeight = "…";
 String lastMiner = "…";

 
 void fetchBitcoinData() {
  if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // Step 1: Fetch Bitcoin Price (USD + CAD)
http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd,cad&include_24hr_change=true");
int httpCode = http.GET();
if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    deserializeJson(doc, payload);

    float usd = doc["bitcoin"]["usd"] | 0.0f;
    float cad = doc["bitcoin"]["cad"] | 0.0f;

    // USD price
    char usdBuf[32];
    format_price_usd(usd, usdBuf, sizeof(usdBuf));  // → "$69,420.13"
    lastPrice = usdBuf;
    if (priceValueLabel) lv_label_set_text(priceValueLabel, usdBuf);

    Serial.print("💰 Bitcoin Price (USD): "); Serial.println(usdBuf);

    // SATS / $1 USD
    if (satsUsdLabel && usd > 0.0f) {
      int sats_usd = (int)(100000000.0f / usd);
      lv_label_set_text_fmt(satsUsdLabel, "%d SATS / $1 USD", sats_usd);
    }

    // CAD price (small line, Row 3)
    if (priceCadLabel) {
      char cadBuf[40];
      char cadCore[32];
      format_price_usd(cad, cadCore, sizeof(cadCore)); // gives "$69,420.13"
      snprintf(cadBuf, sizeof(cadBuf), "CAD %s", cadCore);
      lv_label_set_text(priceCadLabel, cadBuf);

      Serial.print("💵 Bitcoin Price (CAD): "); Serial.println(cadBuf);
    }

    // SATS / $1 CAD
    if (satsCadLabel && cad > 0.0f) {
      int sats_cad = (int)(100000000.0f / cad);
      lv_label_set_text_fmt(satsCadLabel, "%d SATS / $1 CAD", sats_cad);
    }

    float changePct = doc["bitcoin"]["usd_24h_change"] | 0.0f;
    Serial.printf("[price] 24h change (usd): %+.2f%%\n", changePct);
    ui_update_change_pill(changePct);


    // Push to chart (USD for now)
    if (priceChart && priceSeries) {
      lv_chart_set_next_value(priceChart, priceSeries, (int)usd);
      lv_chart_refresh(priceChart);
    }

} else {
    Serial.printf("❌ Failed to fetch Bitcoin price. HTTP=%d\n", httpCode);
}
http.end();



      // Step 2: Fetch Block Height
     
      http.begin("https://mempool.space/api/blocks/tip/height");
      httpCode = http.GET();
      if (httpCode == 200) {
          String blockHeightStr = http.getString();
          lastBlockHeight = blockHeightStr;
          lv_label_set_text(blockValueLabel, lastBlockHeight.c_str());
          lastBlockHeight = blockHeightStr; // Store last block height to be displayed so no blank space is present
          Serial.print("📏 Block Height: ");
          Serial.println(blockHeightStr);
      } else {
          Serial.println("❌ Failed to fetch block height.");
      }
      http.end();

      // Step 3: Fetch Fee Estimates
      
      http.begin("https://mempool.space/api/v1/fees/recommended");
      httpCode = http.GET();
      if (httpCode == 200) {
          String payload = http.getString();
          DynamicJsonDocument doc(512);
          deserializeJson(doc, payload);
          int fastestFee = doc["fastestFee"]; // You can also use "halfHourFee", etc.
          char feeFormatted[16];
          sprintf(feeFormatted, "%d sat/vB", fastestFee);
          lastFee = feeFormatted; // Store last fee to be displayed so no blank space is present
          lv_label_set_text(feeValueLabel, feeFormatted); // Update fee label
          Serial.print("⚡ Fee Estimate: ");
          Serial.println(feeFormatted);
      } else {
          Serial.println("❌ Failed to fetch fee estimates.");
      }
      http.end();

      // Step 4: Fetch Miner Information
      // Get the block hash
      String blockHash = "";
     
      http.begin("https://mempool.space/api/blocks/tip/hash");
      httpCode = http.GET();
      if (httpCode == 200) {
          blockHash = http.getString();
          Serial.println("🔗 Block hash: " + blockHash);
      } else {
          Serial.println("❌ Failed to fetch block hash!");
      }
      http.end();

      if (blockHash != "") {
          // Get the coinbase TXID directly
          String cbTxUrl = "https://mempool.space/api/block/" + blockHash + "/txid/0";
          Serial.println("🌐 Requesting Coinbase TXID: " + cbTxUrl);
          http.begin(cbTxUrl);
          httpCode = http.GET();

          String coinbaseTxId = "";
          if (httpCode == 200) {
              coinbaseTxId = http.getString();
              Serial.print("🔑 Coinbase TXID: ");
              Serial.println(coinbaseTxId);
          } else {
              Serial.println("❌ Failed to fetch coinbase TXID.");
          }
          http.end();

          if (coinbaseTxId != "") {
              // Fetch the coinbase transaction details
              String txUrl = "https://mempool.space/api/tx/" + coinbaseTxId;
              Serial.println("🌐 Fetching coinbase TX: " + txUrl);
              http.begin(txUrl);
              httpCode = http.GET();

              if (httpCode == 200) {
                  String txPayload = http.getString();
                  Serial.println("🧪 Coinbase TX JSON:");
                  Serial.println(txPayload);

                  DynamicJsonDocument txDoc(8192);
                  DeserializationError txError = deserializeJson(txDoc, txPayload);

                  if (txError) {
                      Serial.print("❌ TX JSON Deserialization failed: ");
                      Serial.println(txError.c_str());
                  } else {
                      const char* scriptSig = txDoc["vin"][0]["scriptsig"];
                      String miner = "Unknown";

                      if (scriptSig) {
                          String hexStr = String(scriptSig);
                          Serial.print("📦 scriptsig: ");
                          Serial.println(hexStr);

                          String decoded = hexToAscii(hexStr);
                          Serial.print("🔤 Decoded ASCII: ");
                          Serial.println(decoded);

                          
                          
                          miner = identifyMiner(decoded);
                          lastMiner = miner; // Store last miner to be displayed so no blank space is present
                      } else {
                          Serial.println("⚠️ scriptsig not found!");
                      }

                      lv_label_set_text(solvedByValueLabel, miner.c_str()); // Update miner label
                      Serial.printf("✅ Solved by: %s\n", miner.c_str());
                      
                  }
              } else {
                  Serial.printf("❌ Failed to fetch coinbase TX: %d\n", httpCode);
                  lv_label_set_text(solvedByValueLabel, "Error");
              }
              http.end();
          } else {
              Serial.println("⚠️ Coinbase TXID was empty. Aborting miner lookup.");
          }
      } else {
          Serial.println("⚠️ Invalid block hash. Skipping coinbase lookup.");
      }
  } else {
      Serial.println("❌ WiFi not connected!");
  }
}



//Bitcoin Chart
void fetchBitcoinChartData(lv_chart_series_t* series, lv_obj_t* chart) {
  Serial.println(F("[chart] Fetching 24h prices…"));
  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/coins/bitcoin/market_chart?vs_currency=usd&days=1");
  int httpCode = http.GET();
  Serial.printf("[chart] HTTP: %d\n", httpCode);

  if (httpCode != 200) {
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[chart] JSON error: ");
    Serial.println(err.c_str());
    return;
  }

  JsonArray prices = doc["prices"];
  size_t n = prices.size();
  Serial.printf("[chart] points available: %u\n", (unsigned)n);
  if (n < 2) return;

  const int targetPts = 24;   // show ~24 hourly points
  float first = 0, last = 0;
  float minv = FLT_MAX, maxv = -FLT_MAX;

  // 1) Sample ~24 points evenly across the window, compute min/max
  for (int i = 0; i < targetPts; i++) {
    size_t idx = (size_t)((float)i * (n - 1) / (targetPts - 1));
    float p = prices[idx][1]; // [timestamp, price]
    if (i == 0) first = p;
    if (i == targetPts - 1) last = p;
    if (p < minv) minv = p;
    if (p > maxv) maxv = p;

    if (chart && series) {
      // Footer chart gets the actual price values
      lv_chart_set_value_by_id(chart, series, i, (lv_coord_t)p);
    }
  }

  // 2) Footer chart: apply a little padding so the line isn't touching edges
  if (chart) {
    float pad = (maxv - minv) * 0.05f;
    if (pad < 1) pad = 1; // guard tiny windows
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y,
                       (lv_coord_t)(minv - pad),
                       (lv_coord_t)(maxv + pad));
    lv_chart_refresh(chart);
  }

  // 3) Mini sparkline: normalize to 0..100 and push into priceSeriesMini
  if (priceChartMini && priceSeriesMini) {
    for (int i = 0; i < targetPts; i++) {
      size_t idx = (size_t)((float)i * (n - 1) / (targetPts - 1));
      float p = prices[idx][1];
      float y = (maxv > minv) ? ( (p - minv) * 100.0f / (maxv - minv) ) : 50.0f;
      lv_chart_set_value_by_id(priceChartMini, priceSeriesMini, i, (lv_coord_t)y);
    }
    lv_chart_refresh(priceChartMini);
  }
}



  
 
  
 // NEW: Toggle flags
 //bool isLeftOn = false;  
 //bool isRightOn = false;
 
 
 
 void setup() {
   Serial.begin(115200);
   Serial.println("🚀 Touch Test: Dual Button Toggle Mode");
 
   lcd_init();
 
   Serial.println("Connecting to Wi-Fi...");
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }
   Serial.println("\nWi-Fi connected!");
 
   Serial.println("Display initialized.");

    lvgl_port_lock(-1);
    ui::init_ui_theme();         // ← initialize theme once
    
 
   // ***STYLES***
   lv_style_init(&orangeStyle);
   lv_style_set_bg_color(&orangeStyle, lv_color_hex(0xFFA500));
   lv_style_set_radius(&orangeStyle, 10);
   lv_style_set_bg_opa(&orangeStyle, LV_OPA_COVER);
 
   lv_style_init(&greenStyle);
   lv_style_set_bg_color(&greenStyle, lv_color_hex(0x00FF00));
   lv_style_set_radius(&greenStyle, 10);
   lv_style_set_bg_opa(&greenStyle, LV_OPA_COVER);
 
   lv_style_init(&blueStyle);
   lv_style_set_bg_color(&blueStyle, lv_color_hex(0x007BFF));
   lv_style_set_radius(&blueStyle, 10);
   lv_style_set_bg_opa(&blueStyle, LV_OPA_COVER);
 
   lv_style_init(&yellowStyle);
   lv_style_set_bg_color(&yellowStyle, lv_color_hex(0xFFFF00));
   lv_style_set_radius(&yellowStyle, 10);
   lv_style_set_bg_opa(&yellowStyle, LV_OPA_COVER);
 
    // ✨ Widget style
   lv_style_init(&widgetStyle);
   lv_style_set_radius(&widgetStyle, 16);
   lv_style_set_bg_color(&widgetStyle, lv_color_hex(0xFCA420));
   lv_style_set_bg_opa(&widgetStyle, LV_OPA_10);
   lv_style_set_border_width(&widgetStyle, 3);
   lv_style_set_border_color(&widgetStyle, lv_color_hex(0xFCA420));
   lv_style_set_border_opa(&widgetStyle, LV_OPA_60);
 
   // ✨ Widget2 style
   lv_style_init(&widget2Style);
   lv_style_set_radius(&widget2Style, 16);
   lv_style_set_bg_color(&widget2Style, lv_color_hex(0x777777));
   lv_style_set_bg_opa(&widget2Style, LV_OPA_20);
   lv_style_set_border_width(&widget2Style, 1);
   lv_style_set_border_color(&widget2Style, lv_color_hex(0xFCA420));
   lv_style_set_border_opa(&widget2Style, LV_OPA_60);
 
   // ✨ Glow effect
   lv_style_init(&glowStyle);
   lv_style_set_radius(&glowStyle, 16);
   lv_style_set_shadow_color(&glowStyle, lv_color_hex(0xFCA420));
   lv_style_set_shadow_width(&glowStyle, 15);
   lv_style_set_shadow_opa(&glowStyle, LV_OPA_70);
 
   
   
   lv_scr_load(create_metrics_screen());
 
   lvgl_port_unlock();
 
   Serial.println("✅ Buttons created — touch to toggle colors.");
   Serial.println("Enhanced UI complete.");
   delay(500);           // ✅ Give LVGL time to build UI
   fetchBitcoinData();
   fetchBitcoinChartData(priceSeries, priceChart);  // Initial chart data
   
 }
   
   
   void loop() {
   static unsigned long lastUpdate = 0;
   if (millis() - lastUpdate > 30000) {
     fetchBitcoinData();
     lastUpdate = millis();
   }
   //delay(1000);
 
 }
 
 // --------------------------------------------------
 // 🚀 STACKSWORTH Spark – Engine Activated
 // Built with love, glow effects, and 🍊 coin self-sovereignty
