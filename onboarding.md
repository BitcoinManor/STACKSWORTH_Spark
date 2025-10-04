# STACKSWORTH Spark — Onboarding & Project Overview

> **Current firmware:** `v0.0.3`  
> **What’s new:** Captive-portal Wi-Fi onboarding (SoftAP), SPIFFS-served portal page, and a simple on-device setup screen.

---

## Overview

**STACKSWORTH Spark** is a Bitcoin dashboard for a 7" touchscreen running on an **ESP32-S3**. It shows live Bitcoin metrics (price, block height, mempool fees, latest miner), with a modern LVGL UI and 30-second refresh cadence.

### Hardware
- ESP32-S3 microcontroller (PSRAM recommended)
- 7" 800×480 capacitive touchscreen (Waveshare)
- SPIFFS for portal assets (microSD optional for other assets)
- 2.4 GHz Wi-Fi

### Software
- Arduino (C++)
- LVGL (UI)
- Async stack: **ESPAsyncWebServer** + **DNSServer** for captive portal
- Preferences (NVS) for storing Wi-Fi & device settings

---

## What’s included (v0.0.3)

- **Captive-portal onboarding**
  - SoftAP SSID: `STACKSWORTH-SPARK-<MAC>` (e.g., `STACKSWORTH-SPARK-9C1A2B`)
  - Portal served from SPIFFS: `/STACKS_Wifi_Portal.html(.gz)`
  - `/save` accepts **form** and **JSON**; values stored to Preferences (`ssid`, `password`, `city`, `timezone`, `device`)
  - `/reboot` to restart cleanly; `/ping` for quick health checks
  - Captive DNS redirect (common OSes will auto-open a login sheet)
- **UI pre-screen** in setup mode (shows SSID & `http://192.168.4.1`)
- **Bitcoin metrics screen**
  - Price, block height, mempool fees (sats/vB), latest block miner
  - 30-second auto-refresh, flicker-safe label updates

---

## Quick start

### 1) Build & flash
1. **Partition scheme:** choose one **with SPIFFS**.
2. **SPIFFS upload:** put `STACKS_Wifi_Portal.html.gz` in `/data/`, then upload the filesystem.
3. **Flash firmware:** upload `STACKSWORTH_Spark.ino` at `v0.0.3`.

### 2) First-time setup (no saved Wi-Fi)
1. Device boots into **Setup Mode** and starts an AP:
   - SSID: `STACKSWORTH-SPARK-<MAC>`
   - Default IP: `http://192.168.4.1`
2. Connect from your phone/computer, open `http://192.168.4.1`.
3. Enter Wi-Fi + (optional) City/Timezone/Device, **Save** → unit reboots.
4. After reboot, device connects to your Wi-Fi and loads the dashboard.

> Tip: if a captive window opens but looks blank, close it and use your browser to visit `http://192.168.4.1` directly.

---

## Troubleshooting

- **AP shows but portal won’t load**
  - Try `http://192.168.4.1/ping` → should return `pong`.
  - Ensure `/data/STACKS_Wifi_Portal.html.gz` was uploaded (SPIFFS exists).
- **Portal loads but Save doesn’t stick**
  - Watch Serial for `💾 Saved portal fields`; ensure `/reboot` is hit (the page usually triggers it automatically).
- **Connects to Wi-Fi but no metrics**
  - Some home routers block/inspect HTTPS. Hotspots usually work.
  - (Planned in v0.0.4) Wrap HTTPS fetches with `WiFiClientSecure::setInsecure()` or add NTP + CA bundle.

---

## Code layout (high level)

- `STACKSWORTH_Spark.ino` — boot flow, Wi-Fi/portal, screen selection, data refresh loop  
- `metrics_screen.*` — main dashboard UI (labels, styles, layout, updates)  
- `ui_theme.*` (if present) — theme helpers & shared styles  
- `/data/STACKS_Wifi_Portal.html.gz` — captive portal page (SPIFFS)

---

## Roadmap

- **v0.0.4**: TLS robustness for mempool/price APIs (either `WiFiClientSecure::setInsecure()` or NTP+CA)  
- Weather + city screen, timezone clock  
- Sats per Dollar  
- Optional `/clear` route to wipe Wi-Fi prefs (field service)  
- Pre-screen polish (layout, fonts, remove stray bars)

---

## Credits & License

STACKSWORTH Spark is maintained by the STACKSWORTH team.  
See the **LICENSE** file in this repository for terms.

