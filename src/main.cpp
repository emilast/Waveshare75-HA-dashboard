/* Includes ------------------------------------------------------------------*/
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include <stdlib.h>

#include <WiFiManager.h>
#include <HTTPClient.h>

#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
#define BMP_URL "http://192.168.10.247:5000/" // 24 bpp

uint8_t* buffer = nullptr; // Will be allocated dynamically

bool downloadBMP(const char *url);
void handle24BitImageData(int width, WiFiClient *stream);
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

  printf("EPD_7IN5_V2_test Demo\r\n");
  DEV_Module_Init();

  printf("e-Paper Init and Clear...\r\n");
  EPD_7IN5_V2_Init();
  EPD_7IN5_V2_Clear();
  DEV_Delay_ms(500);

  // Allocate buffer dynamically
  size_t bufferSize = DISPLAY_WIDTH * DISPLAY_HEIGHT / 4;
  buffer = (uint8_t*)malloc(bufferSize);
  if (buffer == nullptr) {
    Serial.println("Failed to allocate buffer memory!");
    ESP.restart();
  }
  Serial.printf("Allocated %d bytes for display buffer\n", bufferSize);

  //Create a new image cache
  /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
  // UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8 ) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
  // if ((buffer = (UBYTE *)malloc(Imagesize)) == NULL) {
  //   printf("Failed to apply for black memory...\r\n");
  //   while (1);
  // }
  printf("Paint_NewImage\r\n");
  Paint_NewImage(buffer, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);


// #if 0   //Partial refresh, example shows time
//     EPD_7IN5_V2_Init_Part();
// 	Paint_NewImage(BlackImage, Font20.Width * 7, Font20.Height, 0, WHITE);
//     Debug("Partial refresh\r\n");
//     Paint_SelectImage(BlackImage);
//     Paint_Clear(WHITE);
	
//     PAINT_TIME sPaint_time;
//     sPaint_time.Hour = 12;
//     sPaint_time.Min = 34;
//     sPaint_time.Sec = 56;
//     UBYTE num = 10;
//     for (;;) {
//         sPaint_time.Sec = sPaint_time.Sec + 1;
//         if (sPaint_time.Sec == 60) {
//             sPaint_time.Min = sPaint_time.Min + 1;
//             sPaint_time.Sec = 0;
//             if (sPaint_time.Min == 60) {
//                 sPaint_time.Hour =  sPaint_time.Hour + 1;
//                 sPaint_time.Min = 0;
//                 if (sPaint_time.Hour == 24) {
//                     sPaint_time.Hour = 0;
//                     sPaint_time.Min = 0;
//                     sPaint_time.Sec = 0;
//                 }
//             }
//         }
//         Paint_ClearWindows(0, 0, Font20.Width * 7, Font20.Height, WHITE);
//         Paint_DrawTime(0, 0, &sPaint_time, &Font20, WHITE, BLACK);

//         num = num - 1;
//         if(num == 0) {
//             break;
//         }
// 		EPD_7IN5_V2_Display_Part(BlackImage, 150, 80, 150 + Font20.Width * 7, 80 + Font20.Height);
//         DEV_Delay_ms(500);//Analog clock 1s
//     }
// #endif
/*
    The feature will only be available on screens sold after 24/10/23
*/

    // EPD_7IN5_V2_Init_4Gray();
    // printf("4 grayscale display\r\n");
    // Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);
    // Paint_SetScale(4);
    // Paint_Clear(0xff);

    // if (GUI_ReadBmp("graph-sample.bmp", 0, 0) != 0) {
    //   printf("Failed to load BMP image\r\n");
    // }

    // Paint_DrawImage(gImage_7in5_V2, 0, 0, EPD_7IN5_V2_WIDTH/2, EPD_7IN5_V2_HEIGHT);

    // // https://github.com/waveshareteam/e-Paper/blob/master/STM32/STM32-F103ZET6/User/Examples/EPD_7in5_V2_test.c
    // EPD_7IN5_V2_Init_Fast();
    // printf("show image for array\r\n");
    // Paint_SelectImage(BlackImage);
    // Paint_Clear(WHITE);
    // Paint_DrawBitMap(graph_sample);
    // EPD_7IN5_V2_Display(BlackImage);
    // DEV_Delay_ms(2000);



    // EPD_7IN5_V2_Init_Fast();
    // EPD_7IN5_V2_Init_4Gray();
    // printf("4 grayscale display\r\n");
    // Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    // printf("show image for array\r\n");
    // Paint_SelectImage(BlackImage);
    // Paint_Clear(WHITE);
    // // Paint_DrawBitMap(gImage_7in5_V2);
    // Paint_DrawBitMap(graph_sample_gray);


    // Paint_DrawPoint(10, 80, GRAY4, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    // Paint_DrawPoint(10, 90, GRAY4, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    // Paint_DrawPoint(10, 100, GRAY4, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    // Paint_DrawLine(420, 70, 70, 120, GRAY4, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(470, 70, 20, 120, GRAY4, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawRectangle(420, 70, 70, 120, GRAY4, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Paint_DrawRectangle(480, 70, 130, 120, GRAY4, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Paint_DrawCircle(45, 95, 20, GRAY4, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Paint_DrawCircle(105, 95, 20, GRAY2, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Paint_DrawLine(85, 95, 125, 95, GRAY4, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    // Paint_DrawLine(105, 75, 105, 115, GRAY4, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    // Paint_DrawString_EN(10, 0, "waveshare", &Font16, GRAY4, GRAY1);
    // Paint_DrawString_EN(10, 20, "hello world", &Font12, GRAY3, GRAY1);
    // Paint_DrawNum(10, 33, 123456789, &Font12, GRAY4, GRAY2);
    // Paint_DrawNum(10, 50, 987654321, &Font16, GRAY1, GRAY4);
    // Paint_DrawString_CN(130, 0,"你好abc", &Font12CN, GRAY4, GRAY1);
    // Paint_DrawString_CN(130, 20,"你好abc", &Font12CN, GRAY3, GRAY2);
    // Paint_DrawString_CN(130, 40,"你好abc", &Font12CN, GRAY2, GRAY3);
    // Paint_DrawString_CN(130, 60,"你好abc", &Font12CN, GRAY1, GRAY4);
    // Paint_DrawString_CN(10, 130, "微雪电子", &Font24CN, GRAY1, GRAY4);
    // EPD_7IN5_V2_WritePicture_4Gray(BlackImage);
    // DEV_Delay_ms(3000);

//   printf("Goto Sleep...\r\n");
// //   EPD_7IN5_V2_Sleep();
//   free(BlackImage);
//   BlackImage = NULL;
}

/* The main loop -------------------------------------------------------------*/
void loop()
{

    if (downloadBMP(BMP_URL)) {
        printf("BMP downloaded successfully!\n");
        // EPD_7IN5_V2_Init_Fast(); // 1-bit
        EPD_7IN5_V2_Init_4Gray();
        Paint_SelectImage(buffer);
        // Paint_Clear(WHITE);
        Paint_DrawBitMap(buffer);
        // EPD_7IN5_V2_Display(buffer); // 1-bit
        // EPD_7IN5_V2_WritePicture_4Gray(buffer); // TODO What does this do?
        EPD_7IN5_V2_Display_4Gray(buffer);

    } else {
        printf("Failed to download BMP image.\n");
    }

  DEV_Delay_ms(60000);

  // delay(60000);

}



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

  // Läs BMP-header (54 byte)
  uint8_t header[54];
  stream->readBytes(header, 54);

  // Enkel kontroll: ska börja med 'BM'
  if (header[0] != 'B' || header[1] != 'M') {
    Serial.println("Not a BMP file");
    http.end();
    return false;
  }

  int dataOffset = *(int*)&header[10];
  int width = *(int*)&header[18];
  int height = *(int*)&header[22];
  uint16_t bitDepth = *(uint16_t*)&header[28];

  Serial.printf("Image downloaded: %dx%d, %d-bit color depth\n", width, abs(height), bitDepth);


  if ((bitDepth != 8 && bitDepth != 24) || width != DISPLAY_WIDTH || abs(height) != DISPLAY_HEIGHT) {
    Serial.println("Incorrect image size or color depth");
    Serial.printf("Expected: %dx%d, 8- or 24-bit color depth\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    Serial.printf("Data offset: %d\n", dataOffset);
    http.end();
    return false;
  }

  // Hoppa till bilddata
  int skip = dataOffset - 54;
  while (skip-- > 0) stream->read(); // Läs bort extra palettdata

  if (bitDepth == 8) {
    handle8BitImageData(width, stream);
  } else if (bitDepth == 24) {
    handle24BitImageData(width, stream);
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


void handle24BitImageData(int width, WiFiClient *stream)
{
  int rowSize = ((width * 3 + 3) / 4) * 4;
  uint8_t row[rowSize];

  // Simple 4x4 Bayer matrix for ordered dithering
  const uint8_t bayer[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
  };

  for (int y = DISPLAY_HEIGHT - 1; y >= 0; y--)
  {
    stream->readBytes(row, rowSize);
    for (int x = 0; x < DISPLAY_WIDTH; x++)
    {
      // BMP stores in BGR order
      uint8_t b = row[x * 3 + 0];
      uint8_t g = row[x * 3 + 1];
      uint8_t r = row[x * 3 + 2];
      // Convert to grayscale (simple average)
      uint8_t gray = (uint16_t(r) + uint16_t(g) + uint16_t(b)) / 3;
      // uint8_t level = gray >> 6; // 0–3
      
      
      // Ordered dithering: adjust gray value by Bayer threshold
      uint8_t threshold = bayer[y % 4][x % 4]; // 0..15
      uint16_t dithered = std::min<int>(255, std::max<int>(0, (gray + threshold) - 8)); // clamp to 0-255
      uint8_t level = dithered >> 6; // 0–3

      int index = y * DISPLAY_WIDTH + x;
      int byteIndex = index / 4;
      int shift = (3 - (index % 4)) * 2;

      buffer[byteIndex] &= ~(0x3 << shift);
      buffer[byteIndex] |= (level << shift);
    }
  }
}

