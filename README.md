# ⚡ STACKSWORTH Spark – Open-Source Bitcoin Dashboard Firmware

Welcome to the official firmware repository for **STACKSWORTH Spark** — a beautifully designed, touchscreen-powered Bitcoin dashboard built on the **ESP32-S3**.

Crafted by [Bitcoin Manor](https://BitcoinManor.com), this project combines performance, usability, and sleek UI design to deliver a plug-and-play Bitcoin experience for every home or workspace.

---

## 🧠 Project Overview

**STACKSWORTH Spark** is a 7-inch smart display that showcases real-time Bitcoin data through a vibrant touchscreen interface. It’s minimal and elegant—no deep menus, just smooth screen transitions and rich data visualization.

This firmware is:
- 🛠️ Built with **Arduino + LVGL**
- 📦 Uses **microSD-stored UI assets**
- 📡 Connected via **Wi-Fi**
- 🧰 Designed for simple, at-home flashing (web flasher, coming soon)

---

## 🚀 Core Features (WIP)

- 📈 Bitcoin Price (USD + CAD) with 24h change
- ⛏️ Latest Block Height + **“Mined by”** pool detection (coinbase tag mapping)
- 🧮 Mempool fees (sat/vB)
- 💸 Sats per Dollar (USD & CAD)
- 🕒 Clock with local timezone support
- 📊 24h price sparkline & footer chart
- 🔁 Auto-rotating or manually navigated screens

---

## 🔧 Hardware Specs

| Component        | Description                                      |
|------------------|--------------------------------------------------|
| 🧠 MCU           | ESP32-S3 (Waveshare 7" Touchscreen model)        |
| 🖥️ Display       | 7" 800×480 capacitive touchscreen (ST7262)       |
| 👆 Touch         | GT911 (via Waveshare display)                    |
| 💽 Storage       | microSD (icons, fonts, backgrounds)              |
| 📶 Connectivity  | Wi-Fi (credentials stored in flash)              |

---

## 🧰 Firmware Stack

- ⚙️ **Language:** Arduino (C++)
- 🎨 **UI:** LVGL
- 🖼️ **Display/Touch:** Waveshare_ST7262_LVGL, ESP32_Display_Panel, ESP32_IO_Expander
- 💾 **Assets:** microSD card
- 🌐 **Flashing:** Web flasher (StacksWorth) — *coming soon*

> 💡 Tip: Pin library versions so the sketch builds consistently across machines (PC, Chromebook, etc.).

---

## 📦 Flash at Home (Coming Soon)

You’ll be able to flash **STACKSWORTH Spark** in your browser—no toolchains required.

👉 Web Flasher (coming soon)  
(We’ll update this section with the live link once published.)

---

## 📜 License

Released under the **MIT License** — use, remix, modify, and sell freely.  
That’s the Bitcoin way: open, sovereign, and unstoppable.

---

### 🧡 Built by Bitcoiners, for Bitcoiners.  
**Bitcoin at a glance.**
