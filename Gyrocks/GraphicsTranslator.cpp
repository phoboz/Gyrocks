/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

//#define FILL_BEFORE_FLUSH

#include "GraphicsTranslator.h"

bool GraphicsTranslatorClass::interpolate_move = true;

void GraphicsTranslatorClass::begin(uint32_t _bufferSize) {
	bufferSize = _bufferSize;
	if (bufferSize < 1024)
		bufferSize = 1024;
	buffer1 = (uint32_t *) malloc(2 * bufferSize * sizeof(uint32_t));
	buffer2 = buffer1 + bufferSize;

  bufferCounter = 0;
  currBuffer = buffer1;

  currX = 0;
  currY = 0;

  currPen = 0;

	// Start DAC
	dac->begin(16);
	dac->setOnTransmitEnd_CB(onTransmitEnd, this);
}

void GraphicsTranslatorClass::end() {
	dac->end();
	free(buffer1);
}

void GraphicsTranslatorClass::flush() {
  if (!bufferCounter)
    return;
    
  while (!dac->canQueue());
  DAC.queueBuffer(currBuffer, bufferCounter);
  
  bufferCounter = 0;
  if (currBuffer == buffer1) {
    currBuffer = buffer2;
  }
  else if (currBuffer == buffer2) {
    currBuffer = buffer1;
  }
}

void GraphicsTranslatorClass::frame_end() {
  currPen = 0;
  currX = 0;
  currY = 0;
#ifdef FILL_BEFORE_FLUSH
  currBuffer[bufferCounter++] = DAC_PACK_COORD(currX, currY);
  flush();
#endif
}

void GraphicsTranslatorClass::pen_enable(uint8_t D) {
  if (currPen != D) {
    currPen = D;
#ifdef FILL_BEFORE_FLUSH
    flush();
#endif
  }
}

void GraphicsTranslatorClass::plot_absolute(uint16_t X, uint16_t Y) {
  int16_t dx, dy;
  int16_t sx, sy;
  uint16_t x0, y0;
  uint16_t x1, y1;

  if (!currPen && !interpolate_move) {
    currX = X;
    currY = Y;
    return;
  }

  x0 = currX;
  y0 = currY;
  x1 = X;
  y1 = Y;
  
  if (x0 <= x1) {
    dx = x1 - x0;
    sx = 1;
  } else {
    dx = x0 - x1;
    sx = -1;
  }

  if (y0 <= y1) {
    dy = y1 - y0;
    sy = 1;
  } else {
    dy = y0 - y1;
    sy = -1;
  }

  int err = dx - dy;

  while (1) {
    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) { // step x
      err = err - dy;
      currX = (x0 += sx);
    }
    if (e2 < dx) { // step y
      err = err + dx;
      currY = (y0 += sy);
    }
  
    currBuffer[bufferCounter++] = DAC_PACK_COORD(currX, currY);
#ifdef FILL_BEFORE_FLUSH
    if (bufferCounter == bufferSize) {
      flush();
    }
#endif
  }
#ifndef FILL_BEFORE_FLUSH
  flush();
#endif
}
 
void GraphicsTranslatorClass::onTransmitEnd(void *_me) {
	GraphicsTranslatorClass *me = reinterpret_cast<GraphicsTranslatorClass *> (_me);
}

GraphicsTranslatorClass GraphicsTranslator(DAC);
