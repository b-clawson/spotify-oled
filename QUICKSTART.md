# Quick Start Guide - Spotify LED Display

Get your Spotify album art display running in 30 minutes.

## Prerequisites

- [ ] ESP32-S3 with 16MB flash, 8MB PSRAM
- [ ] 64x64 HUB75 RGB LED matrix
- [ ] 5V 4A+ power supply
- [ ] Dupont wires
- [ ] Computer with Node.js or Python installed
- [ ] Spotify Premium account (for API access)

---

## Step 1: Wire Hardware (10 min)

Connect HUB75 panel to ESP32-S3:

**Critical connections:**
- All RGB pins: R1, G1, B1, R2, G2, B2
- Row select: A, B, C, D, E
- Control: CLK, LAT, OE
- **Power: 5V supply to panel (NOT from ESP32!)**
- **Ground: Common GND between ESP32 and power supply**

See [docs/PINOUT_REFERENCE.md](docs/PINOUT_REFERENCE.md) for complete pinout.

---

## Step 2: Setup Spotify API (5 min)

1. Go to https://developer.spotify.com/dashboard
2. Create new app:
   - Name: `Spotify LED Display`
   - Redirect URI: `http://localhost:8888/callback`
3. Note your **Client ID** and **Client Secret**

---

## Step 3: Setup Backend Server (10 min)

### Option A: Node.js (Recommended)

```bash
cd backend/nodejs
npm install
cp .env.example .env
nano .env  # Add Client ID and Secret
node get_refresh_token.js  # Follow browser prompts
# Copy refresh token to .env
npm start
```

Server runs on http://localhost:3000

### Option B: Python

```bash
cd backend/python
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
nano .env  # Add Client ID and Secret
python server.py  # Follow browser prompts
```

Server runs on http://localhost:8000

**Test it:** Open http://localhost:3000/now-playing (or :8000) in browser while music plays.

---

## Step 4: Flash ESP32 Firmware (10 min)

```bash
cd firmware
```

Edit `src/main.cpp`:
```cpp
const char* WIFI_SSID = "YourWiFiName";
const char* WIFI_PASSWORD = "YourWiFiPassword";
const char* BACKEND_URL = "http://192.168.1.100:3000/now-playing";
```

**Important:** Use your computer's IP address, not `localhost`!

Find your IP:
- Mac/Linux: `ifconfig | grep inet`
- Windows: `ipconfig`

Flash:
```bash
pio run --target upload
pio device monitor
```

---

## Step 5: Test! (5 min)

1. Power on ESP32 with LED matrix connected
2. Watch serial monitor for "WiFi Connected"
3. Start backend server
4. Play music on Spotify
5. Wait 10 seconds for first update

**Success:** Album art appears on LED matrix!

---

## Troubleshooting

### Display is blank
- Check power supply to LED panel
- Verify all pin connections
- Check serial monitor for errors

### "Can't reach backend"
- Use IP address, not `localhost`
- Check firewall allows port 3000/8000
- Ensure backend server is running

### "WiFi connection failed"
- Verify SSID and password
- Check 2.4GHz network (ESP32 doesn't support 5GHz)

### "Nothing playing"
- Start playing music on Spotify
- Wait up to 10 seconds for poll

---

## Configuration

### Change brightness
Edit `src/main.cpp`:
```cpp
const uint8_t BRIGHTNESS = 64;  // 0-255 (lower = less power)
```

### Change polling speed
Edit `src/main.cpp`:
```cpp
const unsigned long POLL_INTERVAL = 5000;  // 5 seconds
```

### Change backend port
Node.js: Edit `.env` → `PORT=3000`
Python: Edit `.env` → `PORT=8000`

Update firmware `BACKEND_URL` to match!

---

## Next Steps

### Add Real JPEG Decoding

Current firmware shows a gradient placeholder. To display real images:

1. Follow [firmware/INTEGRATION_NOTES.md](firmware/INTEGRATION_NOTES.md)
2. Add TJpg_Decoder library
3. Implement decoder callback
4. Rebuild firmware

### Add Features

- Fade transitions between tracks
- Progress bar showing playback position
- Scrolling text improvements
- Multiple panel support

---

## File Structure

```
spotify-oled/
├── firmware/          # ESP32 code
│   ├── src/main.cpp   # ← Edit WiFi and backend URL here
│   └── platformio.ini
├── backend/
│   ├── nodejs/        # Node.js backend
│   │   ├── .env       # ← Add Spotify credentials here
│   │   └── server.js
│   └── python/        # Python backend alternative
│       ├── .env       # ← Add Spotify credentials here
│       └── server.py
└── docs/
    ├── SETUP.md       # Detailed guide
    └── PINOUT_REFERENCE.md  # Wiring reference
```

---

## Support

- **Wiring issues:** See [docs/PINOUT_REFERENCE.md](docs/PINOUT_REFERENCE.md)
- **Detailed setup:** See [docs/SETUP.md](docs/SETUP.md)
- **JPEG integration:** See [firmware/INTEGRATION_NOTES.md](firmware/INTEGRATION_NOTES.md)

---

## Quick Command Reference

**Backend (Node.js):**
```bash
cd backend/nodejs
npm start
```

**Backend (Python):**
```bash
cd backend/python
source venv/bin/activate
python server.py
```

**Firmware:**
```bash
cd firmware
pio run --target upload     # Flash
pio device monitor          # View logs
```

**Test backend:**
```bash
curl http://localhost:3000/now-playing
```

---

**That's it! Enjoy your visual Spotify experience!** 🎵🖼️
