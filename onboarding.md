# BLOKDBIT Spark - Project Overview

## Project Overview

BLOKDBIT Spark is an open-source Bitcoin dashboard firmware designed for a 7-inch touchscreen display powered by an ESP32-S3 microcontroller. It's a beautiful, real-time Bitcoin data visualization device that can be used in homes or workspaces.

### Core Components

1. **Hardware Platform**:
   - ESP32-S3 microcontroller
   - 7-inch 800x480 capacitive touchscreen (Waveshare model)
   - Onboard microSD card for storing UI assets
   - Wi-Fi connectivity

2. **Software Stack**:
   - Built with Arduino (C++)
   - Uses LVGL (Light and Versatile Graphics Library) for the UI
   - Implements a modular screen-based architecture

### Main Features

The firmware currently implements several key features:

1. **Real-time Bitcoin Data**:
   - Bitcoin price (fetched from CoinGecko API)
   - Current block height
   - Mempool fee estimates
   - Latest block miner identification

2. **UI Components**:
   - Beautiful, modern interface with custom styling
   - Touch-responsive buttons and widgets
   - Glow effects and custom color schemes
   - Modular screen system for easy navigation

3. **Data Updates**:
   - Automatic 30-second refresh cycle for Bitcoin data
   - Wi-Fi connectivity for real-time updates
   - Cached values to prevent display flicker

### Code Structure

The project is organized into several key files:

1. **BLOKDBIT_Spark.ino** (Main firmware):
   - Core initialization and setup
   - Wi-Fi connection handling
   - Bitcoin data fetching logic
   - Main event loop

2. **metrics_screen.h/cpp**:
   - Defines the main metrics display screen
   - Handles UI elements and styling
   - Manages touch events and screen transitions

3. **screen_b.h/cpp**:
   - Additional screen implementation
   - Part of the modular screen system

4. **screen_manager.h/cpp**:
   - Manages screen transitions and navigation
   - Handles screen state

### Key Technical Features

1. **Miner Identification**:
   - Sophisticated system to identify Bitcoin miners from coinbase transactions
   - Maintains a database of known mining pools and their identifiers
   - Decodes hex-encoded miner information

2. **Styling System**:
   - Custom LVGL styles for widgets
   - Multiple color schemes (orange, green, blue, yellow)
   - Glow effects and modern UI elements

3. **Data Management**:
   - Efficient caching of values to prevent display flicker
   - Error handling for API requests
   - Graceful fallbacks when data is unavailable

### Development Status

The project is currently in active development (version v0.0.2) with several features marked as WIP (Work in Progress):
- Weather integration
- Sats per Dollar display
- Clock with timezone support
- Auto-rotating screens

### Getting Started

As a new collaborator, you can:
1. Use the BLOKDBIT Web Flasher (coming soon) to flash the firmware
2. Modify the code using Arduino IDE or your preferred C++ development environment
3. Customize the UI by modifying the LVGL styles and screen layouts
4. Add new features by extending the existing screen system

The project is open-source under the MIT License, allowing for free use, modification, and distribution. 