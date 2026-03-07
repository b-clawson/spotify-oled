# TJpg_Decoder Integration Guide

The current firmware shows a gradient placeholder instead of real album art. This guide explains how to integrate the TJpg_Decoder library to display actual JPEG images.

## Step 1: Add Library Dependency

Edit `platformio.ini` and add TJpg_Decoder to `lib_deps`:

```ini
lib_deps =
    mrcodetastic/ESP32 HUB75 LED MATRIX PANEL DMA Display@^3.0.0
    bblanchon/ArduinoJson@^7.0.0
    bodmer/TJpg_Decoder@^1.0.0
```

## Step 2: Include Headers

Add to `main.cpp`:

```cpp
#include <TJpg_Decoder.h>
```

## Step 3: Implement Decoder Callback

TJpg_Decoder uses a callback function that receives decoded pixel data. Add this function:

```cpp
// Buffer to receive decoded pixels
uint8_t* decodedImageBuffer = nullptr;
int decodeX = 0, decodeY = 0, decodeW = 0, decodeH = 0;

// TJpg_Decoder callback
bool jpegDrawCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // This function is called for each MCU block (typically 8x8 or 16x16 pixels)
    // x, y: position in image
    // w, h: block dimensions
    // bitmap: RGB565 pixel data

    // Store in temporary buffer
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            int srcIdx = dy * w + dx;
            int destX = x + dx;
            int destY = y + dy;

            if (destX < decodeW && destY < decodeH) {
                int destIdx = (destY * decodeW + destX) * 3;
                uint16_t rgb565 = bitmap[srcIdx];

                // Convert RGB565 to RGB888
                decodedImageBuffer[destIdx + 0] = ((rgb565 >> 11) & 0x1F) << 3;
                decodedImageBuffer[destIdx + 1] = ((rgb565 >> 5) & 0x3F) << 2;
                decodedImageBuffer[destIdx + 2] = (rgb565 & 0x1F) << 3;
            }
        }
    }

    return true;
}
```

## Step 4: Initialize Decoder

In `setup()`:

```cpp
void setup() {
    // ... existing setup code ...

    // Initialize TJpg_Decoder
    TJpgDec.setJpgScale(1);  // 1, 2, 4, or 8
    TJpgDec.setSwapBytes(false);
    TJpgDec.setCallback(jpegDrawCallback);

    Serial.println("[TJpg] Decoder initialized");
}
```

## Step 5: Replace Placeholder Decoding

Replace the placeholder code in `downloadAndDecodeImage()`:

```cpp
bool downloadAndDecodeImage(const String& imageUrl) {
    HTTPClient http;

    Serial.println("[HTTP] Downloading image: " + imageUrl);

    http.begin(imageUrl);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.println("[HTTP] Download failed: " + String(httpCode));
        http.end();
        return false;
    }

    int imageSize = http.getSize();
    Serial.println("[HTTP] Image size: " + String(imageSize) + " bytes");

    // Allocate buffer for JPEG data in PSRAM
    uint8_t* jpegBuffer = (uint8_t*)ps_malloc(imageSize);
    if (!jpegBuffer) {
        Serial.println("[ERROR] Failed to allocate JPEG buffer");
        http.end();
        return false;
    }

    // Download to buffer
    WiFiClient* stream = http.getStreamPtr();
    int bytesRead = stream->readBytes(jpegBuffer, imageSize);

    if (bytesRead != imageSize) {
        Serial.println("[ERROR] Incomplete download");
        free(jpegBuffer);
        http.end();
        return false;
    }

    http.end();

    // Get JPEG dimensions
    uint16_t jpegW, jpegH;
    TJpgDec.getJpgSize(&jpegW, &jpegH, jpegBuffer, imageSize);
    Serial.printf("[JPEG] Dimensions: %dx%d\n", jpegW, jpegH);

    // Allocate buffer for decoded RGB888 image
    size_t decodedSize = jpegW * jpegH * 3;
    decodedImageBuffer = (uint8_t*)ps_malloc(decodedSize);
    if (!decodedImageBuffer) {
        Serial.println("[ERROR] Failed to allocate decode buffer");
        free(jpegBuffer);
        return false;
    }

    decodeW = jpegW;
    decodeH = jpegH;
    decodeX = 0;
    decodeY = 0;

    // Decode JPEG
    uint32_t t = millis();
    uint16_t result = TJpgDec.drawJpg(0, 0, jpegBuffer, imageSize);

    if (result != JDR_OK) {
        Serial.printf("[ERROR] JPEG decode failed: %d\n", result);
        free(jpegBuffer);
        free(decodedImageBuffer);
        decodedImageBuffer = nullptr;
        return false;
    }

    Serial.printf("[JPEG] Decoded in %lums\n", millis() - t);
    free(jpegBuffer);

    // Resize to 64x64
    t = millis();
    resizeImage(decodedImageBuffer, jpegW, jpegH, albumArtBuffer, 64, 64, 3);
    Serial.printf("[IMAGE] Resized to 64x64 in %lums\n", millis() - t);

    // Free decoded buffer
    free(decodedImageBuffer);
    decodedImageBuffer = nullptr;

    return true;
}
```

## Step 6: Test

Rebuild and upload:

```bash
pio run --target upload
pio device monitor
```

You should now see real album art!

## Memory Optimization

If you run into memory issues:

### Use Scaled Decoding

TJpg_Decoder can decode at reduced resolution:

```cpp
TJpgDec.setJpgScale(2);  // Decode at 1/2 resolution
TJpgDec.setJpgScale(4);  // Decode at 1/4 resolution
TJpgDec.setJpgScale(8);  // Decode at 1/8 resolution
```

This reduces memory requirements significantly. For 64x64 output, decoding at 1/4 or 1/8 is often sufficient.

### Stream Decoding

Instead of downloading the entire JPEG first, you can stream and decode simultaneously:

```cpp
// Advanced: Implement streaming decoder
class StreamDecoder {
    WiFiClient* client;
public:
    StreamDecoder(WiFiClient* c) : client(c) {}

    static size_t readCallback(JDEC* jd, uint8_t* buff, size_t ndata) {
        StreamDecoder* sd = (StreamDecoder*)jd->device;
        return sd->client->readBytes(buff, ndata);
    }
};
```

## Performance Tips

1. **Pre-allocate buffers** in `setup()` instead of allocating on each download
2. **Use DMA** for image transfer (already done by HUB75 library)
3. **Reduce polling frequency** if needed (10s is reasonable)
4. **Cache images** - only re-download if track changes

## Troubleshooting

### Out of Memory
- Use scaled decoding (`setJpgScale(4)`)
- Free buffers immediately after use
- Check PSRAM is enabled in platformio.ini

### Decode Fails
- Check JPEG is valid (test with image viewer)
- Try different scale factors
- Increase decoder buffer size

### Wrong Colors
- Check `setSwapBytes()` setting
- Verify RGB565 conversion
- Try different dithering algorithms

## Alternative: Use TFT_eSPI's JPEG Support

TFT_eSPI library also has JPEG support:

```cpp
#include <TFT_eSPI.h>

// In TFT_eSPI User_Setup.h:
#define LOAD_JPEG
```

This may be simpler but less flexible.

## Example Serial Output

```
[HTTP] Downloading image: https://i.scdn.co/image/...
[HTTP] Image size: 54832 bytes
[JPEG] Dimensions: 640x640
[JPEG] Decoded in 342ms
[IMAGE] Resized to 64x64 in 23ms
```

## Next Steps

Once JPEG decoding works:
- Add fade transitions between images
- Implement image caching
- Add visual effects (blur, brightness adjust)

---

**For more info:** https://github.com/Bodmer/TJpg_Decoder
