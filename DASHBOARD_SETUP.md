## 📋 STACKSWORTH Spark Dashboard Setup Instructions

### 🚀 **What You Need To Do:**

The dashboard functionality is now implemented in the `.ino` file, but the **dashboard.html** file needs to be uploaded to the device's SPIFFS storage (just like the portal file).

### 🔧 **Steps to Complete Setup:**

1. **Upload the Dashboard HTML:**
   - In Arduino IDE: `Tools` → `ESP32 Sketch Data Upload`
   - This uploads everything in the `/data` folder to the device's SPIFFS
   - **Files that will be uploaded:**
     - `STACKS_Wifi_Portal.html.gz` (existing portal)
     - `dashboard.html` (new dashboard)

2. **Flash the Updated Firmware:**
   - Compile and upload the updated `.ino` file
   - The code now includes mDNS + dashboard server

3. **Test the System:**
   - Connect to WiFi via portal (existing process)
   - Access dashboard at: **`http://spark.local`**
   - Or use device IP: **`http://192.168.x.x`**

### 🌐 **How It Works:**

**Portal Mode (AP):**
- Device creates `STACKSWORTH-SPARK-XXXXXX` network
- Serves `/STACKS_Wifi_Portal.html.gz` from SPIFFS
- User configures WiFi settings

**Dashboard Mode (WiFi Connected):**
- Device connects to home WiFi
- Starts mDNS responder (`spark.local`)
- Serves `/dashboard.html` from SPIFFS
- If dashboard.html missing, shows fallback page with upload instructions

### 📊 **Dashboard Features:**
- **Device Status:** Uptime, memory, WiFi info
- **Network Info:** IP, signal strength, mDNS address
- **Settings:** City, timezone, currency configuration  
- **WiFi Management:** Change networks remotely
- **Live Logs:** Real-time device output (like Serial Monitor)
- **Remote Control:** Reboot device from web interface

### 🔍 **Troubleshooting:**

**If dashboard doesn't load:**
- Check that `ESP32 Sketch Data Upload` was successful
- Look for "Dashboard file missing" message
- Use fallback interface for basic functionality

**If mDNS doesn't work:**
- Try direct IP address instead of `spark.local`
- Some networks block mDNS discovery
- Check device Serial Monitor for mDNS status

### ✅ **Success Indicators:**
- Portal works for initial setup ✅
- Device appears at `http://spark.local` ✅  
- Dashboard shows device information ✅
- Live logs stream in real-time ✅
- Settings can be changed remotely ✅
- WiFi can be reconfigured without reflashing ✅

**You're now ready to test the complete Bitaxe-style dashboard system! 🎉**
