// Rebuild the two mid-strip lines using current swap + theme hex
static void repaint_mid_strip() {
  if (!midLine1Label || !midLine2Label || !g_midLast.valid) return;

  // pull ACC_PRIMARY/SECONDARY into hex strings for recolor tags
  refresh_accent_hex_from_theme();

  const char* hexL1 = g_hex_primary;   // Line 1 → primary (or swapped)
  const char* hexL2 = g_hex_secondary; // Line 2 → secondary (or swapped)

  // Line 1: Hashrate / Difficulty
  const char arrow  = (g_midLast.diffPct > 0) ? '▲' : ((g_midLast.diffPct < 0) ? '▼' : '◆');
  const float absPc = g_midLast.diffPct >= 0 ? g_midLast.diffPct : -g_midLast.diffPct;

  char l1[200];
  snprintf(l1, sizeof(l1),
           "Hashrate: #%s %.0f EH/s#  •  Difficulty: #%s %c%.2f%%%# (#%s %dd#)",
           hexL1, g_midLast.hashrateEh,
           hexL1, arrow, absPc,
           hexL1, g_midLast.diffDaysAgo);
  lv_label_set_text(midLine1Label, l1);

  // Helpers for Line 2
  char capBuf[32];  fmt_usd_compact(g_midLast.marketCapUsd, capBuf, sizeof capBuf);
  unsigned long circ = (g_midLast.circBtc >= 0) ? (unsigned long)(g_midLast.circBtc + 0.5f) : 0;
  char circNum[24]; fmt_int_commas_ul(circ, circNum, sizeof circNum);
  char athBuf[32];  fmt_usd_compact(g_midLast.athUsd, athBuf, sizeof athBuf);

  // Line 2: Market Cap / Circulating / ATH
  char l2[240];
  snprintf(l2, sizeof(l2),
           "Market Cap: #%s %s#  •  Circulating Supply: #%s %s / 21,000,000#  •  ATH: #%s %s# (#%s %dd ago#)",
           hexL2, capBuf,
           hexL2, circNum,
           hexL2, athBuf,
           hexL2, g_midLast.athDaysAgo);
  lv_label_set_text(midLine2Label, l2);
}

// Update other accent-colored widgets on this screen
static void apply_accent_swap_to_widgets() {
  // SATS labels: flip with accents
  if (satsUsdLabel) lv_obj_set_style_text_color(satsUsdLabel, ACC_PRIMARY(), 0);
  if (satsCadLabel) lv_obj_set_style_text_color(satsCadLabel, ACC_SECONDARY(), 0);

  // Price charts: swap series color
  if (priceSeriesMini) { priceSeriesMini->color = ACC_SECONDARY(); if (priceChartMini) lv_chart_refresh(priceChartMini); }
  if (priceSeries)     { priceSeries->color     = ACC_SECONDARY(); if (priceChart)     lv_chart_refresh(priceChart); }

  // 24h pills outlines (optional to tie to accents)
  if (low24Label)  lv_obj_set_style_border_color(low24Label,  ACC_SECONDARY(), 0);
  if (high24Label) lv_obj_set_style_border_color(high24Label, ACC_PRIMARY(),   0);

  // Block card small accents
  if (blocksToCountLabel)  lv_obj_set_style_text_color(blocksToCountLabel,  ACC_PRIMARY(),   0);
  if (solvedByValueLabel)  lv_obj_set_style_text_color(solvedByValueLabel, ACC_SECONDARY(), 0);
}
