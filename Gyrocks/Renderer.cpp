/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "RGBDAC.h"
#include "hershey_font.h"
#include "Renderer.h"

bool RendererClass::interpolate_move = true;

void RendererClass::begin(uint32_t _bufferSize) {
  if (_bufferSize < 1024) {
    bufferSize = 1024;
  }
	bufferSize = _bufferSize;
	buffer = (uint32_t *) malloc(MAX_BUFFERS * bufferSize * sizeof(uint32_t));

  uint32_t *p_buf = buffer;
  for (int i = 0; i < MAX_BUFFERS; i++) {
    buffers[i] = p_buf;
    p_buf += bufferSize;
  }

  character_size(1);
  
  freeBuffers = MAX_BUFFERS;
  bufferCounter = 0;
  currBuffer = buffers[0];

  currX = 0;
  currY = 0;

  currPen = 0;

  pens[0].r = pens[0].g = pens[0].b = 0;
  pens[1].r = pens[1].g = pens[1].b = 255;

  pinMode(Z_BLANK_PIN, OUTPUT);
  digitalWrite(Z_BLANK_PIN, LOW);
  
  setupRGBDAC();
  writeRGB_DACs(0, 0, 0);
  flushedPen = MAX_PENS + 1;

	// Start DAC
	dac->begin(16);
	dac->setOnTransmitEnd_CB(onTransmitEnd, this);
}

void RendererClass::end() {
  frame_end();
	dac->end();
	free(buffer);
}

void RendererClass::nextBuffer() {
  if (!bufferCounter) {
    return;
  }
  
  while (!dac->canQueue());
  
  if (currPen != flushedPen) {
    if (currPen != 0 && flushedPen != 0) {
      writeRGB_DACs(pens[currPen].r, pens[currPen].g, pens[currPen].b); 
      flushedPen = currPen;
    }
  }
  if (currPen) {
    digitalWrite(Z_BLANK_PIN, HIGH);
  }

  DAC.queueBuffer(currBuffer, bufferCounter); 
  noInterrupts();
  freeBuffers--;
  interrupts();

  while (freeBuffers <= 0);
  
  bufferCounter = 0;
  if (currBuffer == buffers[0]) {
    currBuffer = buffers[1];
  }
    if (currBuffer == buffers[1]) {
    currBuffer = buffers[2];
  }
  else if (currBuffer == buffers[2]) {
    currBuffer = buffers[0];
  }
}

void RendererClass::frame_end() {
  nextBuffer();

  pen_enable(0);
  move(0, 0);
}

void RendererClass::pen_enable(uint8_t D) {
  if (D < MAX_PENS) {
    if (currPen != D) {
      nextBuffer();
      currPen = D;
    }
  }
}

void RendererClass::move(uint16_t x, uint16_t y) {
  int counter;
  int len, dwell;
  
  if (interpolate_move) {

    counter = 0;
    while(counter < dwellBeforeMove) {
      currBuffer[bufferCounter++] = DAC_PACK_COORD(currX, currY);
      if (bufferCounter == bufferSize) {
        break;
      }
      counter++;
    }

    len = max(abs(x - currX), abs(y - currY));
    dwell = len >> 2;
  
    currX = x;
    currY = y;

    counter = 0;
    while(counter < dwell) {
      currBuffer[bufferCounter++] = DAC_PACK_COORD(currX, currY);
      counter++;
      if (bufferCounter == bufferSize) {
        break;
      }
    }

    nextBuffer();
  }
  else {
    currX = x;
    currY = y;
  }
}

void RendererClass::line(uint16_t x, uint16_t y) {
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

void RendererClass::plot_absolute(uint16_t X, uint16_t Y) { 
  if (currPen) {
    line(X, Y);
  }
  else {  
    move(X, Y);
  }
}

void RendererClass::character_size(uint8_t D) {
  if (D < 4) {
    fontSizeShift = D;
    fontDirection = FONT_DIR_HORIZONTAL;
  }
  else if (D < 8) {
    fontSizeShift = D - 4;
    fontDirection = FONT_DIR_VERTICAL;
  }  
}

int RendererClass::draw_character_horizontal(char c, int x, int y, uint8_t penID) {
  const hershey_char_t * const f = &hershey_simplex[c - ' '];
  int next_moveto = 1;

  for (int i = 0 ; i < f->count ; i++) {
    int dx = f->points[(i << 1) + 0];
    int dy = f->points[(i << 1) + 1];
    if (dx == -1) {
      next_moveto = 1;
      continue;
    }
    dx = (dx << fontSizeShift);
    dy = (dy << fontSizeShift);
    if (next_moveto) {
      pen_enable(0);
      move(x + dx, dy + y);
    }
    else {
      pen_enable(penID);
      line(x + dx, dy + y);
    }
    next_moveto = 0;
  }
  return (f->width << fontSizeShift);
}

int RendererClass::draw_character_vertical(char c, int x, int y, uint8_t penID) {
  const hershey_char_t * const f = &hershey_simplex[c - ' '];
  int next_moveto = 1;

  for (int i = 0 ; i < f->count ; i++) {
    int dy = f->points[(i << 1) + 0];
    int dx = f->points[(i << 1) + 1];
    if (dy == -1) {
      next_moveto = 1;
      continue;
    }
    dx = (dx << fontSizeShift);
    dy = (dy << fontSizeShift);
    if (next_moveto) {
      pen_enable(0);
      move(x + dx, dy + y);
    }
    else {
      pen_enable(penID);
      line(x + dx, dy + y);
    }
    next_moveto = 0;
  }
  return (f->width << fontSizeShift);
}

void RendererClass::text(const char * s) {
  int x = currX, y = currY;
  uint8_t penID = currPen;

  if (fontDirection == FONT_DIR_HORIZONTAL) {
    while (*s != 0) {
      char c = *s++;
      x += draw_character_horizontal(c, x, y, penID);
    }
  }
  else if (fontDirection == FONT_DIR_VERTICAL) {
    while (*s != 0) {
      char c = *s++;
      y += draw_character_vertical(c, x, y, penID);
    }
  }

  currPen = penID;
}

void RendererClass::pen_RGB(int penID, uint8_t R, uint8_t G, uint8_t B) {
  if (penID > 0 && penID < MAX_PENS) {
    pens[penID].r = R;
    pens[penID].g = G;
    pens[penID].b = B;
  }
}

void RendererClass::onTransmitEnd(void *_me) {
	RendererClass *me = reinterpret_cast<RendererClass *> (_me);
  digitalWrite(Z_BLANK_PIN, LOW);
  
  if (++me->freeBuffers > me->MAX_BUFFERS) {
    me->freeBuffers = me->MAX_BUFFERS;
  }
}

RendererClass Renderer(DAC);
