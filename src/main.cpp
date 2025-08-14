/* Includes ------------------------------------------------------------------*/
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>

#include <WiFiManager.h>
#include <HTTPClient.h>

#define DISPLAY_WIDTH EPD_7IN5_V2_WIDTH
#define DISPLAY_HEIGHT EPD_7IN5_V2_HEIGHT
#define BMP_URL "http://192.168.10.247:5000/3" // 24 bpp

#define IMAGE_WIDTH 600
#define IMAGE_HEIGHT 440


uint8_t* buffer = nullptr; // Will be allocated dynamically
size_t bufferSize = 0;
uint8_t* imageBuffer = nullptr; // Will be allocated dynamically
size_t imageBufferSize = 0;

bool downloadBMP(const char *url);
void handle24BitImageData(int width,int height, WiFiClient *stream, uint8_t* imageBuffer, size_t imageBufferSize);
void handle8BitImageData(int width, WiFiClient *stream);


/* Entry point ----------------------------------------------------------------*/
void setup()
{
    // WiFi-anslutning via WifiManager
  WiFiManager wm;
  bool res = wm.autoConnect("ESP32-Setup");
  if (!res) {
    Serial.println("WiFi connection failed.");
    ESP.restart();
  }
  Serial.println("Connected to WiFi");

  // Allocate display buffer dynamically
  bufferSize = DISPLAY_WIDTH * DISPLAY_HEIGHT / 4;
  buffer = (uint8_t*)malloc(bufferSize);
  if (buffer == nullptr) {
    Serial.println("Failed to allocate display buffer memory!");
    ESP.restart();
  }
  memset(buffer, WHITE, bufferSize);
  Paint_NewImage(buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, WHITE);
  Serial.printf("Allocated %d bytes for display buffer\n", bufferSize);

  // Allocate image buffer dynamically
  imageBufferSize = IMAGE_WIDTH * IMAGE_HEIGHT / 4;
  imageBuffer = (uint8_t*)malloc(imageBufferSize);
  if (imageBuffer == nullptr) {
    Serial.println("Failed to allocate image buffer memory!");
    ESP.restart();
  }
  memset(imageBuffer, WHITE, imageBufferSize);
  Serial.printf("Allocated %d bytes for image buffer\n", imageBufferSize);

  DEV_Module_Init();

  // Startup message
  // EPD_7IN5_V2_Init();
  EPD_7IN5_V2_Init_Fast(); // Fast => Less flicker when displaying the screen
  printf("Startup screen...\r\n");
  Paint_SelectImage(buffer);
  Paint_Clear(WHITE);
  Paint_DrawString_EN(70, 50, "Initializing...", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(70, 70, BMP_URL, &Font12, WHITE, BLACK);
  EPD_7IN5_V2_Display(buffer); // Offsetfel
  // DEV_Delay_ms(2000);

  // // Offsetfel
  // EPD_7IN5_V2_Init_4Gray();
  // printf("4 gray display 1\r\n");
  // Paint_SelectImage(buffer);
  // Paint_Clear(WHITE);
  // Paint_DrawString_EN(400, 50, "waveshare 2", &Font16, BLACK, WHITE);
  // Paint_DrawString_EN(400, 400, "hello world 2", &Font12, WHITE, BLACK);
  // EPD_7IN5_V2_WritePicture_4Gray(buffer); // Känns komprimerat, fel format, grått i kanten på text
  // // EPD_7IN5_V2_Display_4Gray(buffer); // Offsetfel
  // DEV_Delay_ms(2000);

  EPD_7IN5_V2_Sleep();
}

void loop()
{
  EPD_7IN5_V2_Init_4Gray();
  Paint_SelectImage(buffer);
  memset(buffer, WHITE, bufferSize);
  
  if (downloadBMP(BMP_URL)) {
    printf("BMP downloaded successfully!\n");

    // Paint_DrawBitMap(imageBuffer); // Works when image and display are of the same size
    // Paint_NewImage(imageBuffer, IMAGE_WIDTH, IMAGE_HEIGHT, 0, WHITE);
    // Paint_SelectImage(imageBuffer);

    // Paint_DrawImage funkar inte med gråskala?
    Paint_DrawImage(imageBuffer, 
      // 0,0,
      (DISPLAY_WIDTH-IMAGE_WIDTH) /2,
      (DISPLAY_HEIGHT-IMAGE_HEIGHT) /2 + 50,
      IMAGE_WIDTH, IMAGE_HEIGHT*2); // Height * 2 to adjust for gray scale rendering

    EPD_7IN5_V2_Display_4Gray(buffer);
    EPD_7IN5_V2_Sleep(); // Turn off screen power until next call to EPD_7IN5_V2_Init*().
  } else {
    printf("Failed to download BMP image.\n");
  }

  DEV_Delay_ms(60000);
  // delay(60000);
}

// Downloads a BMP image from the given URL and decodes it into the display buffer.
// Supports 8-bit (grayscale) and 24-bit (RGB) BMPs with dimensions matching the display.
bool downloadBMP(const char* url) {
  Serial.printf("Downloading BMP from: %s\n", url);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();

  // Read BMP header (54 bytes)
  uint8_t header[54];
  stream->readBytes(header, 54);

  // Simple check: should start with 'BM'
  if (header[0] != 'B' || header[1] != 'M') {
    Serial.println("Not a BMP file");
    http.end();
    return false;
  }

  int dataOffset = *(int*)&header[10];
  int width = *(int*)&header[18];
  int height = *(int*)&header[22];
  uint16_t bitDepth = *(uint16_t*)&header[28];

  Serial.printf("Image downloaded: %dx%d, %d-bit color depth\n", width, height, bitDepth);

  // Validate image size and color depth
  if ((bitDepth != 8 && bitDepth != 24) || width != IMAGE_WIDTH || height != IMAGE_HEIGHT) {
    Serial.println("Incorrect image size or color depth");
    Serial.printf("Expected: %dx%d, 8- or 24-bit color depth\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    Serial.printf("Data offset: %d\n", dataOffset);
    http.end();
    return false;
  }

  // Skip to image data (skip palette if present)
  int skip = dataOffset - 54;
  while (skip-- > 0) stream->read(); // Skip extra palette data

  // Decode image data into buffer
  if (bitDepth == 8) {
    handle8BitImageData(width, stream);
  } else if (bitDepth == 24) {
    handle24BitImageData(width, height, stream, imageBuffer, imageBufferSize);
  }

  http.end();
  return true;
}

void handle8BitImageData(int width, WiFiClient *stream)
{
  int rowSize = ((width + 3) / 4) * 4;
  uint8_t row[rowSize];

  for (int y = DISPLAY_HEIGHT - 1; y >= 0; y--)
  {
    stream->readBytes(row, rowSize);
    for (int x = 0; x < DISPLAY_WIDTH; x++)
    {
      uint8_t pixel = row[x];     // 0–255
      uint8_t level = pixel >> 6; // 0–3

      int index = y * DISPLAY_WIDTH + x;
      int byteIndex = index / 4;
      int shift = (3 - (index % 4)) * 2;

      buffer[byteIndex] &= ~(0x3 << shift);
      buffer[byteIndex] |= (level << shift);
    }
  }
}


void handle24BitImageData(int width, int height, WiFiClient *stream, uint8_t* imageBuffer, size_t imageBufferSize)
{
  int rowSize = ((width * 3 + 3) / 4) * 4;
  uint8_t row[rowSize];

  memset(imageBuffer, WHITE, imageBufferSize); // Clear image buffer to white

  // Simple 4x4 Bayer matrix for ordered dithering
  const uint8_t bayer[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
  };

  for (int y = height - 1; y >= 0; y--)
  {
    stream->readBytes(row, rowSize);
    for (int x = 0; x < width; x++)
    {
      // BMP stores in BGR order
      uint8_t b = row[x * 3 + 0];
      uint8_t g = row[x * 3 + 1];
      uint8_t r = row[x * 3 + 2];
      // Convert to grayscale (simple average)
      uint8_t gray = (uint16_t(r) + uint16_t(g) + uint16_t(b)) / 3;
      uint8_t level = gray >> 6; // 0–3
      
      // Apply gamma correction to boost midtones and above
      // float gamma = 0.8f; // <1.0 boosts mid/high tones
      // float norm = gray / 255.0f;
      // norm = powf(norm, gamma);
      // uint8_t gamma_gray = uint8_t(norm * 255.0f + 0.5f);

      // Ordered dithering: adjust gray value by Bayer threshold
      // uint8_t threshold = bayer[y % 4][x % 4]; // 0..15
      // uint16_t dithered = std::min<int>(255, std::max<int>(0, (gamma_gray + threshold) - 8)); // clamp to 0-255
      // uint8_t level = dithered >> 6; // 0–3

      int index = y * width + x; // The width here is the display buffer width, not image width
      int byteIndex = index / 4;
      int shift = (3 - (index % 4)) * 2;

      imageBuffer[byteIndex] &= ~(0x3 << shift);
      imageBuffer[byteIndex] |= (level << shift);
    }
  }
}

