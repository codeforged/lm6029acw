#ifndef LM6029ACW_H
#define LM6029ACW_H
 
#include "Arduino.h"
#include "defaultFont.h"
#include <Adafruit_GFX.h>

#define swap(a, b) { uint8_t t = a; a = b; b = t; }
#define _BV(bit) (1 << (bit))
#define LCDWIDTH 128
#define LCDHEIGHT 64 

class LM6029ACW {
  public:
    LM6029ACW(int CS1, int RES, int RS, int WR, int RD, int DB7, int DB6, int DB5, int DB4, int DB3, int DB2, int DB1, int DB0);
    byte ContrastLevel;
    int X,Y;

    uint8_t reverse(uint8_t n);
    void drawChar(uint8_t x, uint8_t line, const char c);
    void drawString(uint8_t x, uint8_t line, char *c);
    void updateScreen();
    void clearRAM();
    void initLCDM();
    void setPixel(const uint8_t x, const uint8_t y, const uint8_t color);    
    void drawCircle(uint8_t x0, uint8_t y0, uint8_t r, 
            uint8_t color);
    void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, 
              uint8_t color);
    void ShowBMP(int x,int y,int width,int high,byte *bmp);
    void setContrast(byte c);
    void drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
    void updatePage(uint8_t page);
  private:
    int _CS, _RES, _RS, _WR, _RD;
    int _DB0,_DB1,_DB2,_DB3,_DB4,_DB5,_DB6,_DB7;
    uint8_t st7565_buffer[1024];    
    
    byte getDataBus();
    void setDataBus(byte i);
    byte RdData();
    void SdData(byte RDData);
    void SdCmd(byte Command);
    void WritePattern(byte *Pattern);    
    void writeScreen(byte *DisplayData);    
};

class LM6029ACW_GFX : public Adafruit_GFX {
public:
    LM6029ACW_GFX(LM6029ACW &lcd) : Adafruit_GFX(128, 64), _lcd(lcd) {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        _lcd.setPixel(x, y, color ? 1 : 0);
    }
    void display() {
        _lcd.updateScreen();
    }
private:
    LM6029ACW &_lcd;
};
 
#endif
