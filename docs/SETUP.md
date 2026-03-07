# Spotify Album Art Display - Complete Setup Guide

This guide walks you through setting up your ESP32-S3 + HUB75 LED matrix display to show Spotify album artwork.

## Table of Contents

1. [Hardware Requirements](#hardware-requirements)
2. [Hardware Wiring](#hardware-wiring)
3. [Spotify Developer Setup](#spotify-developer-setup)
4. [Backend Server Setup](#backend-server-setup)
5. [ESP32 Firmware Setup](#esp32-firmware-setup)
6. [Troubleshooting](#troubleshooting)

---

## Hardware Requirements

### Required Components

| Component | Specification |
|-----------|---------------|
| **Microcontroller** | ESP32-S3 DevKit with 16MB Flash, 8MB PSRAM |
| **Display** | 64x64 RGB LED Matrix Panel, HUB75 interface, P2.5 pixel pitch |
| **Power Supply** | 5V DC, minimum 4A (for full brightness) |
| **Cables** | Dupont jumper wires for connections |

### Optional

- Level shifter (if your ESP32 is 3.3V and matrix expects 5V logic)
- Capacitor (1000μF, 6.3V+) across power supply for stability

---

## Hardware Wiring

### HUB75 to ESP32-S3 Connection

The HUB75 connector has 16 pins. Connect them to your ESP32-S3 as follows:

| HUB75 Pin | Signal | ESP32-S3 GPIO | Notes |
|-----------|--------|---------------|-------|
| R1 | Red (upper half) | GPIO25 | |
| G1 | Green (upper half) | GPIO26 | |
| B1 | Blue (upper half) | GPIO27 | |
| R2 | Red (lower half) | GPIO14 | |
| G2 | Green (lower half) | GPIO12 | |
| B2 | Blue (lower half) | GPIO13 | |
| A | Row select A | GPIO23 | |
| B | Row select B | GPIO19 | |
| C | Row select C | GPIO05 | |
| D | Row select D | GPIO17 | |
| E | Row select E | GPIO18 | For 64-row panels |
| CLK | Clock | GPIO16 | |
| LAT | Latch | GPIO4 | |
| OE | Output Enable | GPIO15 | |
| GND | Ground | GND | Multiple GND pins |

**Important Notes:**
- Connect **all GND pins** from HUB75 to ESP32 GND
- Power the LED panel with a **separate 5V supply** (NOT from ESP32!)
- Connect the 5V supply GND to ESP32 GND (common ground)

### Power Considerations

**LED Matrix Power:**
- 64x64 RGB matrix can draw 3-8A at full white brightness
- Use a quality 5V power supply rated for at least 4A
- Connect power directly to the panel's power input
- **Never power the matrix from ESP32's 5V pin!**

**ESP32 Power:**
- Can be powered via USB-C during development
- For standalone operation, use a separate 5V→3.3V regulator or USB power bank

---

## Spotify Developer Setup

### 1. Create a Spotify App

1. Go to https://developer.spotify.com/dashboard
2. Log in with your Spotify account
3. Click **"Create App"**
4. Fill in the form:
   - **App name:** `Spotify LED Display` (or any name)
   - **App description:** `LED matrix display for album art`
   - **Redirect URI:** `http://localhost:8888/callback`
   - **API/SDKs:** Check "Web API"
5. Click **"Save"**

### 2. Get Your Credentials

1. On your app's dashboard, click **"Settings"**
2. Note your **Client ID** and **Client Secret**
3. You'll need these for the backend server

---

## Backend Server Setup

You can choose either **Node.js** or **Python** for the backend. Pick one:

### Option A: Node.js Backend

#### Prerequisites
- Node.js 18+ installed
- npm or yarn

#### Setup

```bash
cd backend/nodejs

# Install dependencies
npm install

# Copy environment template
cp .env.example .env

# Edit .env and add your Spotify credentials
nano .env
```

Edit `.env`:
```env
SPOTIFY_CLIENT_ID=your_client_id_here
SPOTIFY_CLIENT_SECRET=your_client_secret_here
SPOTIFY_REFRESH_TOKEN=  # Leave empty for now
PORT=3000
```

#### Get Refresh Token

Run the helper script to get your refresh token:

```bash
node get_refresh_token.js
```

Follow the instructions:
1. Open the URL in your browser
2. Authorize the app
3. Copy the refresh token from the terminal
4. Add it to your `.env` file

#### Start the Server

```bash
npm start
```

The server will start on `http://localhost:3000`

---

### Option B: Python Backend

#### Prerequisites
- Python 3.8+ installed
- pip

#### Setup

```bash
cd backend/python

# Create virtual environment
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Copy environment template
cp .env.example .env

# Edit .env and add your Spotify credentials
nano .env
```

Edit `.env`:
```env
SPOTIFY_CLIENT_ID=your_client_id_here
SPOTIFY_CLIENT_SECRET=your_client_secret_here
SPOTIFY_REDIRECT_URI=http://localhost:8888/callback
PORT=8000
```

#### Start the Server (First Time)

```bash
python server.py
```

On first run, your browser will open for Spotify authorization:
1. Click "Agree" to authorize
2. You'll be redirected to localhost
3. The token will be cached in `.spotify_cache`
4. The server will start

For subsequent runs, it will use the cached token automatically.

---

### Test the Backend

Open your browser and go to:
- http://localhost:3000/now-playing (Node.js)
- http://localhost:8000/now-playing (Python)

Play a song on Spotify and refresh the page. You should see JSON with track info.

Example response:
```json
{
  "playing": true,
  "id": "6kex0zd2KGffDZ88cT28F1",
  "artist": "Daft Punk",
  "track": "Digital Love",
  "album": "Discovery",
  "image": "https://i.scdn.co/image/ab67616d0000b273..."
}
```

---

## ESP32 Firmware Setup

### Prerequisites

- **PlatformIO** installed (VS Code extension or CLI)
- USB-C cable for programming ESP32-S3

### Setup

1. **Open the firmware project:**

```bash
cd firmware
```

2. **Edit configuration in `src/main.cpp`:**

```cpp
// WiFi credentials
const char* WIFI_SSID = "YourWiFiNetwork";
const char* WIFI_PASSWORD = "YourWiFiPassword";

// Backend server URL (use your computer's IP, not localhost!)
const char* BACKEND_URL = "http://192.168.1.100:3000/now-playing";
```

**Important:** Replace `192.168.1.100` with your computer's actual IP address on the local network. Find it with:
- **macOS/Linux:** `ifconfig` or `ip addr`
- **Windows:** `ipconfig`

3. **Build and upload:**

```bash
# Using PlatformIO CLI
pio run --target upload

# Or use VS Code PlatformIO extension:
# Click "Upload" in the PlatformIO toolbar
```

4. **Monitor serial output:**

```bash
pio device monitor

# Or in VS Code: click "Serial Monitor"
```

You should see:
```
[WiFi] Connected!
[WiFi] IP: 192.168.1.50
[System] Ready!
[HTTP] Polling backend...
```

---

## Testing End-to-End

1. **Start the backend server** (Node.js or Python)
2. **Power on the ESP32** with the LED matrix connected
3. **Play a song on Spotify**
4. **Watch the display:**
   - After ~10 seconds, the album art should appear
   - Track name scrolls at the bottom

---

## Configuration Options

### Adjust Polling Interval

In `src/main.cpp`:
```cpp
const unsigned long POLL_INTERVAL = 10000;  // milliseconds (10s)
```

### Adjust Display Brightness

In `src/main.cpp`:
```cpp
const uint8_t BRIGHTNESS = 128;  // 0-255
```

Lower brightness = less power draw and cooler panel.

### Change Backend Port

**Node.js:**
Edit `.env`:
```env
PORT=3000
```

**Python:**
Edit `.env`:
```env
PORT=8000
```

Remember to update the `BACKEND_URL` in ESP32 firmware!

---

## Troubleshooting

### LED Matrix Issues

| Problem | Solution |
|---------|----------|
| **Display is blank** | Check power supply and all wiring connections |
| **Wrong colors** | Check R1/G1/B1/R2/G2/B2 pin connections |
| **Flickering** | Check CLK, LAT, OE pins; try lower brightness |
| **Horizontal lines** | Check row select pins (A, B, C, D, E) |
| **Display shifted/garbled** | Check HUB75 cable connection, verify pin mapping |

### ESP32 Issues

| Problem | Solution |
|---------|----------|
| **Won't connect to WiFi** | Double-check SSID and password, verify 2.4GHz network |
| **Can't reach backend** | Use IP address, not localhost; check firewall |
| **Upload fails** | Hold BOOT button while uploading; check USB cable |
| **Out of memory** | Firmware uses PSRAM; ensure `board_build.psram_type = opi` in platformio.ini |

### Backend Issues

| Problem | Solution |
|---------|----------|
| **Token expired** | Node.js: regenerate refresh token; Python: delete `.spotify_cache` |
| **Rate limited** | Reduce polling frequency (increase POLL_INTERVAL) |
| **No track info** | Verify music is playing on Spotify; check OAuth scopes |
| **Connection refused** | Check firewall allows incoming connections on port 3000/8000 |

### Image Display Issues

| Problem | Solution |
|---------|----------|
| **Shows gradient instead of art** | JPEG decoder not integrated yet; see firmware TODO |
| **Images look pixelated** | Expected on 64x64; this is max resolution |
| **Colors look off** | Adjust gamma correction or RGB565 conversion |

---

## Next Steps

### Integrate JPEG Decoder

The current firmware shows a placeholder gradient. To display real images:

1. **Add TJpg_Decoder library** to `platformio.ini`:
```ini
lib_deps =
    mrcodetastic/ESP32 HUB75 LED MATRIX PANEL DMA Display@^3.0.0
    bblanchon/ArduinoJson@^7.0.0
    bodmer/TJpg_Decoder@^1.0.0
```

2. **Implement decoding** in `downloadAndDecodeImage()` function
3. **Decode to RGB888**, then resize and convert to RGB565

Reference: https://github.com/Bodmer/TJpg_Decoder

### Add Features

- **Fade transitions** between album covers
- **Progress bar** showing playback position
- **Animated visualizations** when paused
- **Multiple panel support** (128x64, 128x128)

---

## Support

For issues or questions:
- ESP32-HUB75 library: https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA
- Spotify API docs: https://developer.spotify.com/documentation/web-api

---

## License

MIT
