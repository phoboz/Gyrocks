#ifndef _RGBDAC_H
#define _RGBDAC_H

#include "Arduino.h"

void setupRGBDAC();
void writeRGB_DACs(uint8_t r, uint8_t g, uint8_t b);

#endif
