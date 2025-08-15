/* Includes ------------------------------------------------------------------*/
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>

#include <WiFiManager.h>
#include <HTTPClient.h>

#define DISPLAY_WIDTH EPD_7IN5_V2_WIDTH
#define DISPLAY_HEIGHT EPD_7IN5_V2_HEIGHT


// Array of strings BMP_URL_1 and BMP_URL_2 as items
const char* bmpUrls[] = {
   "http://192.168.10.247:5000/1",
   "http://192.168.10.247:5000/2"
};


int currentUrlIndex = 0;

uint8_t* buffer = nullptr; // Will be allocated dynamically
size_t bufferSize = 0;

bool downloadBMP(const char *url);
void handle8BitImageData(int width, WiFiClient *stream);
void handle24BitImageData(int width, int height, WiFiClient *stream, int x, int y, uint8_t* buffer, size_t bufferSize);


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

  // Allocate image buffer dynamically
  bufferSize = DISPLAY_WIDTH * DISPLAY_HEIGHT / 4;
  buffer = (uint8_t*)malloc(bufferSize);
  if (buffer == nullptr) {
    Serial.println("Failed to allocate buffer memory!");
    ESP.restart();
  }
  memset(buffer, 0xFF, bufferSize);
  Paint_NewImage(buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, WHITE);
  Serial.printf("Allocated %d bytes for display buffer\n", bufferSize);

  DEV_Module_Init();

  // Startup message
  // EPD_7IN5_V2_Init();
  EPD_7IN5_V2_Init_Fast(); // Fast => Less flicker when displaying the screen
  printf("Startup screen...\r\n");
  Paint_SelectImage(buffer);
  Paint_Clear(WHITE);
  Paint_DrawString_EN(70, 50, "Initializing...", &Font16, WHITE, BLACK);

  int yStart = 70;
  for (const char* url : bmpUrls) {
    Paint_DrawString_EN(70, yStart, url, &Font12, WHITE, BLACK);
    yStart += 20;
  }

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

void EPD_7IN5_V2_Clear_4Gray(void) {
    // size = width * height / 4 because 4 pixels per byte in 4-gray mode
    uint32_t size = EPD_7IN5_V2_WIDTH * EPD_7IN5_V2_HEIGHT / 4;
    uint8_t white = 0xFF; // 0b11111111 means all pixels = white (2 bits each)

    uint8_t *clearBuffer = (uint8_t *)malloc(size);
    if (clearBuffer == NULL) return;

    memset(clearBuffer, white, size);

    EPD_7IN5_V2_Display_4Gray(clearBuffer);

    free(clearBuffer);
}


void loop()
{
  EPD_7IN5_V2_Init_4Gray();
  // EPD_7IN5_V2_Clear_4Gray();
  Paint_SelectImage(buffer);
  Paint_Clear(BLACK);

  // Iterate over
  const char* url = bmpUrls[currentUrlIndex];
  currentUrlIndex = (currentUrlIndex + 1) % 2;

  if (downloadBMP(url)) {
      printf("BMP downloaded successfully!\n");
      // Paint_DrawBitMap(buffer);
      EPD_7IN5_V2_Display_4Gray(buffer);
      EPD_7IN5_V2_Sleep(); // Turn off screen power until next call to EPD_7IN5_V2_Init*().
  }
    else {
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
  if ((bitDepth != 8 && bitDepth != 24)) {
    Serial.println("Incorrect image size or color depth");
    Serial.printf("Expected: %dx%d, 8- or 24-bit color depth\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
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
    handle24BitImageData(width, height, stream, 
      (DISPLAY_WIDTH-width)/2,
      (DISPLAY_HEIGHT-height)/2 + 20,
      buffer, bufferSize);
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


void handle24BitImageData(int width, int height, WiFiClient *stream, int xStart, int yStart, uint8_t* buffer, size_t bufferSize)
{
  int rowSize = ((width * 3 + 3) / 4) * 4;
  uint8_t row[rowSize];

  memset(buffer, 0xFF, bufferSize);

  for (int y = height - 1; y >= 0; y--)
  {
    // if (y % 10 == 0) printf("Processing row %d. Free heap: %d\n", y, ESP.getFreeHeap());
  
    stream->readBytes(row, rowSize);

    for (int x = 0; x < width; x++)
    {
      // BMP stores in BGR order
      uint8_t b = row[x * 3 + 0];
      uint8_t g = row[x * 3 + 1];
      uint8_t r = row[x * 3 + 2];
      // Convert to grayscale (simple average)
      uint8_t gray = (uint16_t(r) + uint16_t(g) + uint16_t(b)) / 3;
      uint8_t level = level = gray >> 6; // 0–3

      int index = (yStart + y) * DISPLAY_WIDTH + (xStart + x);
      int byteIndex = index / 4;
      int shift = (3 - (index % 4)) * 2;

      buffer[byteIndex] &= ~(0x3 << shift);
      buffer[byteIndex] |= (level << shift);
    }
  }
}

