#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"
#include "TouchScreen.h"

#define TFT_CS  10
#define TFT_DC  9

Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);

#define YP A0
#define XM A1
#define YM 7
#define XP 8

#define TS_MINX 110
#define TS_MINY 80
#define TS_MAXX 900
#define TS_MAXY 940

#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define BOXSIZE 40 //
#define PENRADIUS 4

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
int oldColor; //
int currentColor = HX8357_RED; //


void setup() {
  tft.begin();

  tft.fillScreen(HX8357_BLACK);

  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, HX8357_RED);
  tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, HX8357_YELLOW);
  tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, HX8357_GREEN);
  tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, HX8357_CYAN);
  tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, HX8357_BLUE);
  tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, HX8357_MAGENTA);
  tft.fillRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, HX8357_BLACK);
  tft.fillRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
}


void loop(void) {
 
  //* Second
  TSPoint p = ts.getPoint();
 
  if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
     return;
  }  

  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  
  if (p.y - PENRADIUS > BOXSIZE) {
    tft.fillCircle(p.x, p.y, PENRADIUS, currentColor);
  }
  else {
    oldColor = currentColor;

    if (p.x < BOXSIZE) { 
      currentColor = HX8357_RED; 
      tft.drawRect(0, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
    } else if (p.x < BOXSIZE*2) {
      currentColor = HX8357_YELLOW;
      tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
    } else if (p.x < BOXSIZE*3) {
      currentColor = HX8357_GREEN;
      tft.drawRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
    } else if (p.x < BOXSIZE*4) {
      currentColor = HX8357_CYAN;
      tft.drawRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
    } else if (p.x < BOXSIZE*5) {
      currentColor = HX8357_BLUE;
      tft.drawRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
    } else if (p.x < BOXSIZE*6) {
      currentColor = HX8357_MAGENTA;
      tft.drawRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
    } else if (p.x < BOXSIZE*7) {
      currentColor = HX8357_WHITE;
      tft.drawRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, HX8357_RED);
    } else if (p.x < BOXSIZE*8) {
      currentColor = HX8357_BLACK;
      tft.drawRect(BOXSIZE*7, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
    }

    if (oldColor != currentColor) {
      if (oldColor == HX8357_RED) 
        tft.fillRect(0, 0, BOXSIZE, BOXSIZE, HX8357_RED);
      if (oldColor == HX8357_YELLOW) 
        tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, HX8357_YELLOW);
      if (oldColor == HX8357_GREEN) 
        tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, HX8357_GREEN);
      if (oldColor == HX8357_CYAN) 
        tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, HX8357_CYAN);
      if (oldColor == HX8357_BLUE) 
        tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, HX8357_BLUE);
      if (oldColor == HX8357_MAGENTA) 
        tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, HX8357_MAGENTA);
      if (oldColor == HX8357_WHITE) 
        tft.fillRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
      if (oldColor == HX8357_BLACK) 
        tft.fillRect(BOXSIZE*7, 0, BOXSIZE, BOXSIZE, HX8357_BLACK);
    }
  }
  //*/
}
