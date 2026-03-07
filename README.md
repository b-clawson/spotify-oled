# Spotify Album Art Display - HUB75 RGB LED Matrix

Display the currently playing Spotify album artwork on a 64x64 RGB LED matrix, powered by ESP32-S3.

![Project Overview](https://img.shields.io/badge/ESP32--S3-64x64%20RGB%20Matrix-blue)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

- **Full-color album artwork** on 64x64 RGB LED matrix
- **Standalone operation** - ESP32 connects via WiFi
- **Automatic updates** - polls every 10 seconds
- **Scrolling track info** at the bottom
- **Easy setup** - simple backend server (Node.js or Python)
- **Low latency** - album art appears within seconds of track change

---

## System Architecture

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│   Spotify   │◄────────┤   Backend    │◄────────┤   ESP32-S3  │
│     API     │ OAuth   │   Server     │  WiFi   │  + HUB75    │
│             │         │  (Node/Py)   │         │  LED Matrix │
└─────────────┘         └──────────────┘         └─────────────┘
                              │
                              │ Exposes
                              ▼
                        GET /now-playing
                        {
                          "artist": "...",
                          "track": "...",
                          "album": "...",
                          "image": "https://..."
                        }
```

**Flow:**
1. Backend authenticates with Spotify OAuth
2. Backend polls Spotify "Currently Playing" endpoint
3. ESP32 polls backend via HTTP every 10 seconds
4. ESP32 downloads album art from Spotify CDN
5. ESP32 resizes image to 64x64 and displays on LED matrix

---

## Hardware

### Components

| Part | Description |
|------|-------------|
| **ESP32-S3** | 16MB Flash, 8MB PSRAM (e.g., ESP32-S3-DevKitC-1) |
| **LED Matrix** | 64x64 RGB HUB75 panel, P2.5 pixel pitch |
| **Power Supply** | 5V, 4A+ (for LED matrix) |
| **Cables** | Dupont wires for HUB75 connection |

### Why ESP32-S3?

- **8MB PSRAM** - stores decoded images (64x64 RGB = ~12KB)
- **WiFi** - downloads images over the network
- **Fast CPU** - handles JPEG decoding and image processing
- **DMA support** - smooth LED matrix refresh via I2S

---

## Quick Start

### 1. Hardware Setup

Wire the HUB75 panel to ESP32-S3 (see [docs/SETUP.md](docs/SETUP.md) for pinout).

### 2. Backend Setup

Choose Node.js **or** Python:

**Node.js:**
```bash
cd backend/nodejs
npm install
cp .env.example .env
# Edit .env with Spotify credentials
node get_refresh_token.js
npm start
```

**Python:**
```bash
cd backend/python
pip install -r requirements.txt
cp .env.example .env
# Edit .env with Spotify credentials
python server.py
```

### 3. Firmware Setup

```bash
cd firmware
# Edit src/main.cpp:
#   - Set WiFi SSID/password
#   - Set BACKEND_URL to your computer's IP
pio run --target upload
pio device monitor
```

### 4. Play Music!

Open Spotify and play a song. The album art will appear on the matrix within 10 seconds.

---

## Project Structure

```
spotify-oled/
├── firmware/                 # ESP32-S3 Arduino code
│   ├── platformio.ini
│   └── src/
│       └── main.cpp          # Main firmware
├── backend/
│   ├── nodejs/               # Node.js backend option
│   │   ├── package.json
│   │   ├── server.js
│   │   ├── get_refresh_token.js
│   │   └── .env.example
│   └── python/               # Python FastAPI backend option
│       ├── requirements.txt
│       ├── server.py
│       └── .env.example
├── docs/
│   └── SETUP.md              # Detailed setup guide
└── README_HUB75.md           # This file
```

---

## API Endpoints (Backend)

### `GET /now-playing`

Returns currently playing track information.

**Response (playing):**
```json
{
  "playing": true,
  "id": "6kex0zd2KGffDZ88cT28F1",
  "artist": "Daft Punk",
  "track": "Digital Love",
  "album": "Discovery",
  "image": "https://i.scdn.co/image/ab67616d0000b273...",
  "progress_ms": 45000,
  "duration_ms": 239000
}
```

**Response (nothing playing):**
```json
{
  "playing": false
}
```

### `GET /health`

Health check endpoint.

**Response:**
```json
{
  "status": "ok",
  "uptime": 12345
}
```

---

## Configuration

### Firmware (`firmware/src/main.cpp`)

```cpp
// WiFi credentials
const char* WIFI_SSID = "your_wifi_ssid";
const char* WIFI_PASSWORD = "your_wifi_password";

// Backend URL (use your computer's IP!)
const char* BACKEND_URL = "http://192.168.1.100:3000/now-playing";

// Polling interval (ms)
const unsigned long POLL_INTERVAL = 10000;  // 10 seconds

// Display brightness (0-255)
const uint8_t BRIGHTNESS = 128;
```

### Backend

**Node.js** (`.env`):
```env
SPOTIFY_CLIENT_ID=your_client_id
SPOTIFY_CLIENT_SECRET=your_secret
SPOTIFY_REFRESH_TOKEN=your_refresh_token
PORT=3000
```

**Python** (`.env`):
```env
SPOTIFY_CLIENT_ID=your_client_id
SPOTIFY_CLIENT_SECRET=your_secret
SPOTIFY_REDIRECT_URI=http://localhost:8888/callback
PORT=8000
```

---

## TODO / Future Enhancements

### Priority
- [ ] Integrate **TJpg_Decoder** for real JPEG decoding (currently shows placeholder)
- [ ] Implement **fade transitions** between album covers
- [ ] Add **progress bar** showing playback position

### Nice to Have
- [ ] **Text overlay** for artist/track name
- [ ] **Animated background** when paused
- [ ] **Multiple panel support** (128x64, 128x128)
- [ ] **OTA updates** for firmware
- [ ] **Web config portal** (WiFi Manager)
- [ ] **Brightness auto-adjust** based on time of day

---

## Troubleshooting

### Display is blank
- Check 5V power supply and connections
- Verify HUB75 wiring matches pinout
- Check serial monitor for error messages

### ESP32 won't connect to WiFi
- Verify SSID and password
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check signal strength

### No album art appears
- Verify backend is running and accessible
- Check `BACKEND_URL` uses IP address, not "localhost"
- Play music on Spotify
- Check serial monitor for HTTP errors

### Colors look wrong
- Verify R1/G1/B1/R2/G2/B2 pin connections
- Check for loose wires

See [docs/SETUP.md](docs/SETUP.md) for complete troubleshooting guide.

---

## Development

### Building

```bash
cd firmware
pio run
```

### Uploading

```bash
pio run --target upload
```

### Monitoring

```bash
pio device monitor
```

### Clean Build

```bash
pio run --target clean
```

---

## Performance

- **Polling frequency:** Adjustable, default 10 seconds
- **Image download time:** ~1-2 seconds (depends on network)
- **Image processing:** <100ms (resize + display)
- **Memory usage:** ~4KB for 64x64 RGB565 framebuffer (stored in PSRAM)
- **Power consumption:**
  - ESP32: ~500mA
  - LED Matrix: 2-8A depending on brightness and content

---

## Credits

- **ESP32-HUB75-MatrixPanel-DMA** by mrcodetastic: https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA
- **Spotify Web API**: https://developer.spotify.com/documentation/web-api
- **Arduino JSON**: https://arduinojson.org

---

## License

MIT License - See LICENSE file for details

---

## Support

For detailed setup instructions, see [docs/SETUP.md](docs/SETUP.md)

For issues with:
- **Firmware/Hardware:** Check ESP32-HUB75 library issues
- **Spotify API:** See Spotify Web API documentation
- **This project:** Open an issue on GitHub

---

**Enjoy your Spotify visual experience!** 🎵🖼️
