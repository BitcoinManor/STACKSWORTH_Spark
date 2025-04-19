# ⚡ BLOKDBIT Spark – Open-Source Bitcoin Dashboard Firmware

Welcome to the official firmware repository for **BLOKDBIT Spark** — a beautifully designed, touchscreen-powered Bitcoin dashboard built on the **ESP32-S3**.

Crafted by [Bitcoin Manor](https://github.com/BitcoinManor), this project combines performance, usability, and sleek UI design to deliver a plug-and-play Bitcoin experience for every home or workspace.

---

## 🧠 Project Overview

**BLOKDBIT Spark** is a 7-inch smart display that showcases real-time Bitcoin data through a vibrant touchscreen interface. It's designed to be minimal, elegant, and easy to use — with no menus, just smooth screen transitions and rich data visualization.

This firmware is:
- 🛠️ Built with **Arduino + LVGL** for performance
- 🎨 Powered by **microSD-stored UI assets**
- 📡 Connected via **Wi-Fi**
- 💡 Designed for **home flashing** via the [BLOKDBIT Web Flasher](https://bitcoinmanor.github.io/BlokdBit-Matrix/)

---

## 🚀 Core Features (WIP)

- 📈 Bitcoin Price (via CoinGecko or Mempool.space)
- ⛏️ Block Height & Mempool stats
- 🌦️ Real-time Weather
- 💸 Sats per Dollar
- 🕒 Clock with local timezone support
- 🔁 Auto-rotating or manually navigated screens

---

## 🔧 Hardware Specs

| Component | Description |
|----------|-------------|
| **Microcontroller** | ESP32-S3 (Waveshare 7" Touchscreen Model) |
| **Display** | 7" 800x480 capacitive touchscreen |
| **Storage** | Onboard microSD (for icons, fonts, backgrounds) |
| **Connectivity** | Wi-Fi (credentials saved in flash) |

---

## 🧰 Firmware Stack

- ⚙️ **Language:** Arduino (C++)
- 🎨 **UI Engine:** LVGL (manually configured)
- 📦 **Graphics Driver:** LovyanGFX or TFT_eSPI
- 💾 **Asset Storage:** microSD card
- 🌐 **Flashing:** Compatible with BLOKDBIT Web Flasher

---

## 📦 Flash at Home (Coming Soon)

Soon you'll be able to flash your BLOKDBIT Spark firmware with no setup required using our browser-based tool:

👉 **[BLOKDBIT Web Flasher](https://bitcoinmanor.github.io/BLOKDBIT_WebFlasher/)**  
*(This will be updated to include Spark firmware)*

---

## 📜 License

This project is released under the **MIT License** — use, remix, modify, and sell freely.  
That’s the **Bitcoin way**: open, sovereign, and unstoppable.

---

### 🧡 Built by Bitcoiners, for Bitcoiners.  
Together, we're making sound money visible.


