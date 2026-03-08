/*
 * Spotify Album Art Display - ESP32-S3 + HUB75 64x64 RGB LED Matrix
 * COMPLETE WORKING VERSION
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <TJpg_Decoder.h>
#include "spotify_logo.h"

// =============================================================================
// CONFIGURATION
// =============================================================================

const char* WIFI_SSID = "Clawson (New) HOME";
const char* WIFI_PASSWORD = "342westjamesstreet";
const char* BACKEND_URL = "https://spotify-oled-production.up.railway.app/now-playing";

const unsigned long POLL_INTERVAL = 3000;  // 3 seconds - faster updates!
const uint8_t BRIGHTNESS = 64;

#define PANEL_RES_X 64
#define PANEL_RES_Y 64
#define PANEL_CHAIN 1

MatrixPanel_I2S_DMA *dma_display = nullptr;

// =============================================================================
// Global Variables
// =============================================================================

String currentTrackId = "";
unsigned long lastPollTime = 0;
uint16_t* albumArtBuffer = nullptr;

// Scrolling text
String scrollText = "";
int scrollPosition = 0;
unsigned long lastScrollTime = 0;
const unsigned long SCROLL_DELAY = 50;

// =============================================================================
// Color Utilities
// =============================================================================

uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// =============================================================================
// Matrix Initialization - CONFIRMED WORKING PINS
// =============================================================================

void initMatrix() {
    HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);

    // Confirmed working pin mapping
    mxconfig.gpio.r1 = 8;
    mxconfig.gpio.g1 = 9;
    mxconfig.gpio.b1 = 3;
    mxconfig.gpio.r2 = 14;
    mxconfig.gpio.g2 = 12;
    mxconfig.gpio.b2 = 13;
    mxconfig.gpio.a = 11;
    mxconfig.gpio.b = 10;
    mxconfig.gpio.c = 5;
    mxconfig.gpio.d = 6;
    mxconfig.gpio.e = 7;
    mxconfig.gpio.lat = 4;
    mxconfig.gpio.oe = 15;
    mxconfig.gpio.clk = 16;
    mxconfig.clkphase = false;

    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(BRIGHTNESS);
    dma_display->clearScreen();
}

// =============================================================================
// WiFi Connection
// =============================================================================

void connectWiFi() {
    Serial.println("[WiFi] Connecting to " + String(WIFI_SSID));

    dma_display->clearScreen();
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 2);
    dma_display->setTextColor(dma_display->color565(255, 255, 255));
    dma_display->println("WiFi");
    dma_display->println("Connecting...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());

        dma_display->clearScreen();
        dma_display->setCursor(2, 2);
        dma_display->setTextColor(dma_display->color565(0, 255, 0));
        dma_display->println("WiFi OK");
        dma_display->setTextColor(dma_display->color565(255, 255, 255));
        dma_display->setCursor(2, 12);
        dma_display->setTextSize(1);
        dma_display->println(WiFi.localIP().toString());
        delay(2000);
    } else {
        Serial.println("\n[WiFi] Failed!");
        dma_display->clearScreen();
        dma_display->setCursor(2, 2);
        dma_display->setTextColor(dma_display->color565(255, 0, 0));
        dma_display->println("WiFi");
        dma_display->println("Failed!");
        while(1) delay(1000);
    }
}

// =============================================================================
// Image Processing
// =============================================================================

// TJpg_Decoder callback - called during JPEG decode to output pixels
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // Crop/scale if needed, but for 64x64 images we can just display directly
    if (y >= 64) return true;  // Skip if outside display

    for (int16_t j = 0; j < h; j++) {
        if (y + j >= 64) break;
        for (int16_t i = 0; i < w; i++) {
            if (x + i >= 64) break;

            uint16_t rgb565 = bitmap[j * w + i];

            // Convert RGB565 to RGB888 for the display
            uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;
            uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
            uint8_t b = (rgb565 & 0x1F) << 3;

            dma_display->drawPixelRGB888(x + i, y + j, r, g, b);
        }
    }
    return true;
}

void resizeImage(uint8_t* src, int srcW, int srcH, uint16_t* dst, int dstW, int dstH, int channels) {
    float xRatio = (float)srcW / dstW;
    float yRatio = (float)srcH / dstH;

    for (int y = 0; y < dstH; y++) {
        for (int x = 0; x < dstW; x++) {
            int srcX = (int)(x * xRatio);
            int srcY = (int)(y * yRatio);

            if (srcX >= srcW) srcX = srcW - 1;
            if (srcY >= srcH) srcY = srcH - 1;

            int srcIdx = (srcY * srcW + srcX) * channels;
            uint8_t r = src[srcIdx];
            uint8_t g = src[srcIdx + 1];
            uint8_t b = src[srcIdx + 2];

            dst[y * dstW + x] = rgb888to565(r, g, b);
        }
    }
}

bool downloadAndDecodeImage(const String& imageUrl) {
    WiFiClientSecure client;
    client.setInsecure();  // Skip SSL certificate validation

    HTTPClient http;
    Serial.println("[HTTP] Downloading: " + imageUrl);

    http.begin(client, imageUrl);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.println("[HTTP] Failed: " + String(httpCode));
        http.end();
        return false;
    }

    int imageSize = http.getSize();
    Serial.println("[HTTP] Size: " + String(imageSize) + " bytes");

    // Try PSRAM first, fallback to regular RAM
    uint8_t* jpegBuffer = nullptr;
    if (psramFound()) {
        jpegBuffer = (uint8_t*)ps_malloc(imageSize);
        if (jpegBuffer) {
            Serial.println("[Memory] Allocated " + String(imageSize) + " bytes from PSRAM");
        }
    }

    if (!jpegBuffer) {
        jpegBuffer = (uint8_t*)malloc(imageSize);
        if (jpegBuffer) {
            Serial.println("[Memory] Allocated " + String(imageSize) + " bytes from RAM");
        }
    }

    if (!jpegBuffer) {
        Serial.println("[ERROR] Out of memory (tried PSRAM and RAM)");
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    int bytesRead = stream->readBytes(jpegBuffer, imageSize);

    if (bytesRead != imageSize) {
        Serial.println("[ERROR] Incomplete download");
        free(jpegBuffer);
        http.end();
        return false;
    }

    http.end();

    // Decode JPEG using TJpg_Decoder
    Serial.println("[IMAGE] Decoding JPEG...");

    // Set the output function callback
    TJpgDec.setCallback(tft_output);

    // Decode the JPEG image directly to the display
    uint32_t dt = millis();
    TJpgDec.drawJpg(0, 0, jpegBuffer, bytesRead);

    dt = millis() - dt;
    Serial.printf("[IMAGE] Decoded in %d ms\n", dt);

    free(jpegBuffer);
    return true;
}

// =============================================================================
// Display Functions
// =============================================================================

void displayAlbumArt() {
    if (!albumArtBuffer) return;

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            uint16_t rgb565 = albumArtBuffer[y * 64 + x];
            uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;
            uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
            uint8_t b = (rgb565 & 0x1F) << 3;
            dma_display->drawPixelRGB888(x, y, r, g, b);
        }
    }
}

void displayScrollingText(const String& text, int yPos) {
    if (text.length() == 0) return;

    // Clear text area
    for (int y = yPos; y < yPos + 8; y++) {
        for (int x = 0; x < 64; x++) {
            dma_display->drawPixelRGB888(x, y, 0, 0, 0);
        }
    }

    // Draw text
    dma_display->setTextSize(1);
    dma_display->setTextColor(dma_display->color565(255, 255, 255));
    dma_display->setCursor(scrollPosition, yPos);
    dma_display->print(text);

    // Update scroll position
    unsigned long now = millis();
    if (now - lastScrollTime > SCROLL_DELAY) {
        scrollPosition--;
        int textWidth = text.length() * 6;
        if (scrollPosition < -textWidth) {
            scrollPosition = 64;
        }
        lastScrollTime = now;
    }
}

void drawSpotifyLogo() {
    // Display embedded Spotify logo image
    dma_display->clearScreen();

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            uint16_t rgb565 = pgm_read_word(&SPOTIFY_LOGO[y * 64 + x]);

            // Convert RGB565 to RGB888 for display
            uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;
            uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
            uint8_t b = (rgb565 & 0x1F) << 3;

            dma_display->drawPixelRGB888(x, y, r, g, b);
        }
    }
}

void displayFallback() {
    drawSpotifyLogo();
}

// =============================================================================
// Backend Communication
// =============================================================================

bool pollBackend() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Not connected!");
        return false;
    }

    Serial.println("[WiFi] Connected - IP: " + WiFi.localIP().toString());
    Serial.println("[HTTP] Connecting to: " + String(BACKEND_URL));

    WiFiClientSecure client;
    client.setInsecure();  // Skip SSL certificate validation

    HTTPClient http;
    http.begin(client, BACKEND_URL);
    http.setTimeout(10000);  // Increased timeout to 10 seconds

    Serial.println("[HTTP] Sending GET request...");
    int httpCode = http.GET();

    Serial.println("[HTTP] Response code: " + String(httpCode));

    if (httpCode != HTTP_CODE_OK) {
        Serial.println("[HTTP] Backend failed!");
        if (httpCode == -1) {
            Serial.println("[HTTP] Connection failed - check network/firewall");
        }
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.println("[JSON] Parse error: " + String(error.c_str()));
        return false;
    }

    if (!doc["playing"].as<bool>()) {
        Serial.println("[Spotify] Nothing playing");
        currentTrackId = "";
        displayFallback();
        return true;
    }

    String trackId = doc["id"].as<String>();
    String artist = doc["artist"].as<String>();
    String track = doc["track"].as<String>();
    String imageUrl = doc["image"].as<String>();

    Serial.println("[Spotify] Track: " + artist + " - " + track);

    if (trackId != currentTrackId) {
        Serial.println("[Spotify] New track!");
        currentTrackId = trackId;

        // Download and decode - TJpgDec draws directly to display
        downloadAndDecodeImage(imageUrl);
        scrollText = track + " - " + artist;
        scrollPosition = 64;
    }

    return true;
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=================================");
    Serial.println("Spotify Album Art Display");
    Serial.println("ESP32-S3 + HUB75 LED Matrix");
    Serial.println("=================================\n");

    // Allocate buffer
    albumArtBuffer = (uint16_t*)malloc(64 * 64 * sizeof(uint16_t));
    if (!albumArtBuffer) {
        Serial.println("[ERROR] Memory allocation failed!");
        while(1) delay(1000);
    }
    Serial.println("[Memory] Buffer allocated (8KB)");

    // Initialize display
    Serial.println("[Display] Initializing matrix...");
    initMatrix();
    Serial.println("[Display] Ready!");

    // Initialize JPEG decoder
    Serial.println("[JPEG] Initializing decoder...");
    TJpgDec.setJpgScale(1);  // 1:1 scale (no scaling)
    TJpgDec.setSwapBytes(false);  // RGB565 byte order
    Serial.println("[JPEG] Decoder ready!");

    // Connect WiFi
    connectWiFi();

    // Ready message - show Spotify logo
    drawSpotifyLogo();

    Serial.println("\n[System] Ready!");
    Serial.println("[System] Backend: " + String(BACKEND_URL));
    delay(1000);
}

// =============================================================================
// Loop
// =============================================================================

void loop() {
    unsigned long now = millis();

    // Poll backend
    if (now - lastPollTime > POLL_INTERVAL) {
        pollBackend();
        lastPollTime = now;
    }

    // Scrolling text disabled - showing album art only
    // if (scrollText.length() > 0) {
    //     displayScrollingText(scrollText, 56);
    // }

    delay(10);
}
