
// Code by David Galloway
// (c) 2019

#include "RGBDAC.h"

///////////////////////////////////////////////////////////////////
// TLC7225 RGB DAC
// 8 bit DAC inputs
#define RGB_D0 22
#define RGB_D1 24
#define RGB_D2 26
#define RGB_D3 28
#define RGB_D4 30
#define RGB_D5 32
#define RGB_D6 33
#define RGB_D7 34

//Load DAC. A high level simultaneously loads all four DAC registers. DAC registers are transparent when LDAC is low.
#define RGB_LDAC 39
//Write input selects DAC transparency or latch mode
#define RGB_WR 35

//select which of 4 DACs
#define RGB_A0 37
#define RGB_A1 36

static const int RED = 0;
static const int GREEN = 1;
static const int BLUE = 2;
static const int SIZE = 3;

static int rgb_dataPins[] = {RGB_D0, RGB_D1, RGB_D2, RGB_D3, RGB_D4, RGB_D5, RGB_D6, RGB_D7};

static void writeDACregister(uint8_t dac, uint8_t value)
{
  digitalWrite(RGB_A0, (dac&1) );
  digitalWrite(RGB_A1, (dac>>1 & 1));

  for (int i=0;i<8;i++)
  {
    digitalWrite(rgb_dataPins[i], value & 1);
    value = value>>1;
  }   

  digitalWrite(RGB_WR, 0);
  digitalWrite(RGB_WR, 1);
}

void setupRGBDAC() 
{
  for (int i=0;i<8;i++)
  {
    pinMode(rgb_dataPins[i], OUTPUT);
  }

  pinMode(RGB_LDAC, OUTPUT);
  pinMode(RGB_WR, OUTPUT);

  pinMode(RGB_A0, OUTPUT);
  pinMode(RGB_A1, OUTPUT); 

  digitalWrite(RGB_LDAC, 1);
  digitalWrite(RGB_WR, 1);
  
  writeDACregister(RED,0);
  writeDACregister(GREEN,0);
  writeDACregister(BLUE,0);  
  writeDACregister(SIZE,175);  // default to max (SIZE won't be changed in current design)
  
  digitalWrite(RGB_LDAC, 0);
  digitalWrite(RGB_LDAC, 1); 
}


void writeRGB_DACs(uint8_t r, uint8_t g, uint8_t b)
{
  writeDACregister(0,r);
  writeDACregister(1,g);
  writeDACregister(2,b);

  digitalWrite(RGB_LDAC, 0);
  digitalWrite(RGB_LDAC, 1); 
}
