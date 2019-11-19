/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "GraphicsTranslator.h"

bool GraphicsTranslatorClass::interpolate_move = true;
const int GraphicsTranslatorClass::dwellAfterMove = 10;

void GraphicsTranslatorClass::begin(uint32_t _bufferSize) {
	bufferSize = _bufferSize;
	buffer = (uint32_t *) malloc(MAX_BUFFERS * bufferSize * sizeof(uint32_t));

  uint32_t *p_buf = buffer;
  for (int i = 0; i < MAX_BUFFERS; i++) {
    buffers[i] = p_buf;
    p_buf += bufferSize;
  }
  
  freeBuffers = MAX_BUFFERS;
  bufferCounter = 0;
  currBuffer = buffers[0];

  currX = 0;
  currY = 0;

  currPen = 0;

	// Start DAC
	dac->begin(16);
	dac->setOnTransmitEnd_CB(onTransmitEnd, this);
}

void GraphicsTranslatorClass::end() {
	dac->end();
	free(buffer);
}

void GraphicsTranslatorClass::nextBuffer() {
  if (!bufferCounter) {
    return;
  }

  while (!dac->canQueue());
  noInterrupts();
  DAC.queueBuffer(currBuffer, bufferCounter);
  freeBuffers--;
  interrupts();

  while (freeBuffers <= 0);
  
  bufferCounter = 0;
  if (currBuffer == buffers[0]) {
    currBuffer = buffers[1];
  }
  else if (currBuffer == buffers[1]) {
    currBuffer = buffers[0];
  }
}

void GraphicsTranslatorClass::frame_end() {
  nextBuffer();
  
  currPen = 0;
  currX = 0;
  currY = 0;
}

void GraphicsTranslatorClass::pen_enable(uint8_t D) {
  if (currPen != D) {
    currPen = D;
    //nextBuffer();
  }
}

void GraphicsTranslatorClass::move(uint16_t x, uint16_t y) {
  if (interpolate_move) {
    currX = x;
    currY = y;
      
    for (int i; i < dwellAfterMove; i++) {
      currBuffer[bufferCounter++] = DAC_PACK_COORD(currX, currY);
      if (bufferCounter == bufferSize) {
        nextBuffer();
      }
    }    
  }
  else {
    currX = x;
    currY = y;
  }
}

void GraphicsTranslatorClass::line(uint16_t x, uint16_t y) {
  int16_t dx, dy;
  int16_t sx, sy;
  uint16_t x0, y0;
  uint16_t x1, y1;

  x0 = currX;
  y0 = currY;
  x1 = x;
  y1 = y;
  
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
    if (bufferCounter == bufferSize) {
      nextBuffer();
    }
  }
  nextBuffer();
}

void GraphicsTranslatorClass::plot_absolute(uint16_t X, uint16_t Y) { 
  if (currPen) {
    line(X, Y);
  }
  else {  
    move(X, Y);
  }
}

void GraphicsTranslatorClass::onTransmitEnd(void *_me) {
	GraphicsTranslatorClass *me = reinterpret_cast<GraphicsTranslatorClass *> (_me);

  if (++me->freeBuffers > me->MAX_BUFFERS) {
    me->freeBuffers = me->MAX_BUFFERS;
  }
}

GraphicsTranslatorClass GraphicsTranslator(DAC);
