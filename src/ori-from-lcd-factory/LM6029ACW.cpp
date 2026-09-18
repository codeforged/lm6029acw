#include "LM6029ACW.h"
//#include "Arduino.h"
 
/* ***************************************** Public Functions ***************************************** */
 
LM6029ACW::LM6029ACW(int CS1, int RES, int RS, int WR, int RD, int DB7, int DB6, int DB5, int DB4, int DB3, int DB2, int DB1, int DB0) {
    _CS = CS1;
    _RES = RES;
    _RS = RS;
    _WR = WR;
    _RD = RD;
    _DB7 = DB7;
    _DB6 = DB6;
    _DB5 = DB5;
    _DB4 = DB4;
    _DB3 = DB3;
    _DB2 = DB2;
    _DB1 = DB1;
    _DB0 = DB0;

    pinMode(_RS, OUTPUT);
    pinMode(_CS, OUTPUT);
    pinMode(_RES, OUTPUT);
    pinMode(_WR, OUTPUT);
    pinMode(_RD, OUTPUT);

    pinMode(_DB0, OUTPUT);
    pinMode(_DB1, OUTPUT);
    pinMode(_DB2, OUTPUT);
    pinMode(_DB3, OUTPUT);
    pinMode(_DB4, OUTPUT);
    pinMode(_DB5, OUTPUT);
    pinMode(_DB6, OUTPUT);
    pinMode(_DB7, OUTPUT);

    digitalWrite(_CS, 1);  
    digitalWrite(_RES,1);  
    digitalWrite(_RS,1);  
    digitalWrite(_WR,1);  
    digitalWrite(_RD,1);    
}
 
void LM6029ACW::initLCDM() {    
    //SP=0x60;
    //EA = 0;         // disable interrupts
    //noInterrupts();

    digitalWrite(_CS, 1);  
    digitalWrite(_RES,1);  
    digitalWrite(_RS,1);  
    digitalWrite(_WR,1);  
    digitalWrite(_RD,1);

    digitalWrite(_RES, 1);                 // hardware reset LCD module
    digitalWrite(_RES, 0);
    delay(5);
    digitalWrite(_RES, 1);
    delay(10);

    ContrastLevel=0x17;     // default Contrast Level

    SdCmd(0xaf);            // display on
    SdCmd(0x40);            // display start line=0
    SdCmd(0xa0);            // ADC=0
    //SdCmd(0xa1);            // ADC=1 (Horizontal Mirror Flip)
    SdCmd(0xa6);            // normal display
    SdCmd(0xa4);            // Duisplay all point = off
    SdCmd(0xa2);            // LCD bias = 1/9
    SdCmd(0xc8);            // Common output mode select= reverse
    //SdCmd(0xc7);            // Common output mode select= reverse (Vertical Mirror Flip)
    SdCmd(0x2f);            // Power control = all on

    SdCmd(0x25);          // set the Rab ratio to middle
    SdCmd(0x81);            // E-Vol setting
    SdCmd(ContrastLevel);   // (2byte command)
}

void LM6029ACW::setContrast(byte c) {
    SdCmd(0x81);            // E-Vol setting
    SdCmd(c);   // (2byte command)
}
byte LM6029ACW::getDataBus() {
  byte result = 0;
  pinMode(_DB0, INPUT);
  pinMode(_DB1, INPUT);
  pinMode(_DB2, INPUT);
  pinMode(_DB3, INPUT);
  pinMode(_DB4, INPUT);
  pinMode(_DB5, INPUT);
  pinMode(_DB6, INPUT);
  pinMode(_DB7, INPUT);
  
  result =  digitalRead(_DB0); 
  result += digitalRead(_DB1)*2; 
  result += digitalRead(_DB2)*4; 
  result += digitalRead(_DB3)*8; 
  result += digitalRead(_DB4)*16;
  result += digitalRead(_DB5)*32; 
  result += digitalRead(_DB6)*64; 
  result += digitalRead(_DB7)*128; 
  
  pinMode(_DB0, OUTPUT);
  pinMode(_DB1, OUTPUT);
  pinMode(_DB2, OUTPUT);
  pinMode(_DB3, OUTPUT);
  pinMode(_DB4, OUTPUT);
  pinMode(_DB5, OUTPUT);
  pinMode(_DB6, OUTPUT);
  pinMode(_DB7, OUTPUT);
  
  return result;
}

void LM6029ACW::setDataBus(byte i) {
  digitalWrite(_DB0, bitRead(i, 0));
  digitalWrite(_DB1, bitRead(i, 1));
  digitalWrite(_DB2, bitRead(i, 2));
  digitalWrite(_DB3, bitRead(i, 3));
  digitalWrite(_DB4, bitRead(i, 4));
  digitalWrite(_DB5, bitRead(i, 5));
  digitalWrite(_DB6, bitRead(i, 6));
  digitalWrite(_DB7, bitRead(i, 7));/**/
}


byte LM6029ACW::RdData() {
  byte DData;
  digitalWrite(_RS, 1);                   
  digitalWrite(_WR, 1);                   
  digitalWrite(_CS, 0); 
  setDataBus(0xff);    
  digitalWrite(_RD, 0);        
  DData = getDataBus();
  digitalWrite(_RD, 1); 
  digitalWrite(_CS, 0); 
  return(DData);         
}

void LM6029ACW::SdData(byte DData) {           //send data
  digitalWrite(_RS,1);
  digitalWrite(_WR,1);
  digitalWrite(_RD,1);
  setDataBus(DData);    
  digitalWrite(_CS,0);
  digitalWrite(_WR,0);
  /*__asm__("nop \r\n");
  __asm__("nop \r\n");
  __asm__("nop \r\n");
  __asm__("nop \r\n");*/
  digitalWrite(_WR,1);
  digitalWrite(_CS,1);
}


void LM6029ACW::SdCmd(byte Command)   //send command
{
    digitalWrite(_RS,0);
    digitalWrite(_WR,1);
    digitalWrite(_RD,1);
    setDataBus(Command);  
    digitalWrite(_CS,0);
    digitalWrite(_WR,0);
    /*__asm__("nop \r\n");
    __asm__("nop \r\n");
    __asm__("nop \r\n");
    __asm__("nop \r\n");*/
    digitalWrite(_WR,1);
    digitalWrite(_CS,1);

}

void LM6029ACW::WritePattern(byte *Pattern) {
  byte TempData;
  byte i, j;
  for(i=0;i<8;i++)  {
    SdCmd(0xb0 | i);    // select page 0~7
    SdCmd(0x10);        // start form column 4
    SdCmd(0x00);        // (2byte command)
    for(j=0;j<64;j++) {
      TempData=(*(Pattern));
      SdData(TempData);
      TempData=(*(Pattern+1));
      SdData(TempData);
    }
  }
}

void LM6029ACW::clearRAM() {
 memset(st7565_buffer, 0, sizeof(st7565_buffer));
 X=0;
 Y=0;
}

static unsigned char lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

uint8_t LM6029ACW::reverse(uint8_t n) {
   // Reverse the top and bottom nibble then swap them.
   return (lookup[n&0b1111] << 4) | lookup[n>>4];
}

void LM6029ACW::drawChar(uint8_t x, uint8_t line, const char c) {
  for (uint8_t i=0; i<5; i++ ) {
    st7565_buffer[x + (line*128) ] = reverse(pgm_read_byte(defaultFont+(c*5)+i));
    x++;    
  }
}

void LM6029ACW::drawString(uint8_t x, uint8_t line, char *c) {
  while (c[0] != 0) {
    drawChar(x, line, c[0]);
    c++;
    x += 6; // 6 pixels wide
    if (x + 6 >= LCDWIDTH) {
      x = 0;    // ran out of this line
      line++;
    }
    if (line >= (LCDHEIGHT/8))
      line=0;        // ran out of space :(
    X=x;
    Y=line;   
  }
}

void LM6029ACW::updateScreen() {
  writeScreen(st7565_buffer);
}

void LM6029ACW::writeScreen(byte *DisplayData) // DisplayData should be 164x64/8 = 1312byte
{
  byte TempData;
  byte i, j;
  for(i=0;i<8;i++) {
    SdCmd(0xb0 | i);    // select page 0~7
    SdCmd(0x10);        // start form column 4
    SdCmd(0x00);        // (2byte command)
    
    for(j=0;j<128;j++) {
      TempData=(*(DisplayData+(i*128)+j));
      SdData(TempData);
    }
    
    /*SdCmd(0x10-4);        // 4 kolom terakhir
    SdCmd(0x00);        // (2byte command)
    for(j=0;j<128;j++) {
      TempData=(*(DisplayData+(i*128)+j));
      SdData(TempData);
    }*/
  }
}

void LM6029ACW::setPixel(const uint8_t x, const uint8_t y, const uint8_t color) {
  /*if (x>123) {
    if (color) 
      st7565_buffer[4+(x-128)+(y/8)*128] |= _BV((y%8));  
    else
      st7565_buffer[4+(x-128)+(y/8)*128] &= ~_BV((y%8));
  } else */
  if (x>127 || y>63 || x<0 || y<0) return;
  {
    if (color) 
      st7565_buffer[x+(y/8)*128] |= _BV((y%8));  
    else
      st7565_buffer[x+(y/8)*128] &= ~_BV((y%8));
  }
}

void LM6029ACW::drawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t color) {  
  int8_t f = 1 - r;
  int8_t ddF_x = 1;
  int8_t ddF_y = -2 * r;
  int8_t x = 0;
  int8_t y = r;

  setPixel(x0, y0+r, color);
  setPixel(x0, y0-r, color);
  setPixel(x0+r, y0, color);
  setPixel(x0-r, y0, color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    setPixel(x0 + x, y0 + y, color);
    setPixel(x0 - x, y0 + y, color);
    setPixel(x0 + x, y0 - y, color);
    setPixel(x0 - x, y0 - y, color);
    
    setPixel(x0 + y, y0 + x, color);
    setPixel(x0 - y, y0 + x, color);
    setPixel(x0 + y, y0 - x, color);
    setPixel(x0 - y, y0 - x, color);
    
  }
}

void LM6029ACW::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, 
              uint8_t color) {
  uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  uint8_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int8_t err = dx / 2;
  int8_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;}

  for (; x0<=x1; x0++) {
    if (steep) {
      setPixel(y0, x0, color);
    } else {
      setPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void LM6029ACW::ShowBMP(int x,int y,int width,int high,byte *bmp) {
  byte TempData;
  byte i,j;
  int k=0;
  x=x+0x04;                 // LM6038
  for(i=0;i<high/8;i++) {
  SdCmd(y|0xb0);         // ÉèÖÃÒ³µØÖ·
  SdCmd((x>>4)|0x10);    // ÉèÖÃÁÐµØÖ·žß4Î»
  SdCmd(x&0x0f);         // ÉèÖÃÁÐµØÖ·µÍ4Î»
  for(j=0;j<width;j++) {
    TempData=(*(bmp+(i*128)+j));
    SdData(TempData);
    k=k+1;
  }
  y=y+1;                 // Ò³µØÖ·ÐÞÕý
  }/**/
}

void LM6029ACW::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
  // Draw top and bottom
  drawLine(x, y, x + w - 1, y, color);
  drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
  // Draw left and right
  drawLine(x, y, x, y + h - 1, color);
  drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void LM6029ACW::updatePage(uint8_t page) {
    // Set page address (0xB0 | page)
    SdCmd(0xB0 | (page & 0x0F));
    // Set column address to 0
    SdCmd(0x10); // High nibble
    SdCmd(0x00); // Low nibble
    // Kirim 128 byte data untuk page ini
    for (uint8_t col = 0; col < 128; col++) {
        SdData(st7565_buffer[page * 128 + col]);
    }
}