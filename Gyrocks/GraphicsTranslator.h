/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef GRAPHICS_TRANSLATOR_H
#define GRAPHICS_TRANSLATOR_H

#include "Arduino.h"
#include "DAC.h"

class GraphicsTranslatorClass {
public:
	GraphicsTranslatorClass(DACClass &_dac) : dac(&_dac) { };
	void begin(uint32_t _bufferSize);
	void end();
  void frame_end();

  void pen_enable(uint8_t D);
  void plot_absolute(uint16_t X, uint16_t Y);

  static bool interpolate_move;

private:
	static void onTransmitEnd(void *me);
	void flush();
    
	uint32_t bufferSize;
	uint32_t *buffer1;
  uint32_t *buffer2;
  
  uint32_t bufferCounter;
  uint32_t *currBuffer;

  uint16_t currX, currY;
  uint8_t currPen;

	DACClass *dac;
};

extern GraphicsTranslatorClass GraphicsTranslator;

#endif
