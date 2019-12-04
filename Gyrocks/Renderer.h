/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef RENDERER_H
#define RENDERER_H

#include "Arduino.h"
#include "DAC.h"

class RendererClass {
public:
	RendererClass(DACClass &_dac) : dac(&_dac) { };
	void begin(uint32_t _bufferSize);
	void end();
  void frame_end();

  void pen_enable(uint8_t D);
  void plot_absolute(uint16_t X, uint16_t Y);

  void character_size(uint8_t D);
  void text(const char * s);

  void pen_RGB(int penID, uint8_t R, uint8_t G, uint8_t B);
  
  static bool interpolate_move;

private:
	static void onTransmitEnd(void *me);
	void nextBuffer();
 
  void line(uint16_t x, uint16_t y);
  void move(uint16_t x, uint16_t y);

  int draw_character_horizontal(char c, int x, int y, uint8_t penID);
  int draw_character_vertical(char c, int x, int y, uint8_t penID);
  
  static const int MAX_BUFFERS = 3;
  static const int dwellBeforeMove = 10;
  static const int Z_BLANK_PIN = 38;
  static const int MAX_PENS = 12;

  unsigned int fontSizeShift;
  enum FontDirection {
    FONT_DIR_HORIZONTAL,
    FONT_DIR_VERTICAL
  } fontDirection;
  
	uint32_t bufferSize;
  uint32_t *buffer;
	uint32_t *buffers[MAX_BUFFERS];

  volatile int freeBuffers;
  int bufferCounter;
  uint32_t *currBuffer;

  struct PenStruct {
    uint8_t r, g, b;
  } pens[MAX_PENS];

  uint16_t currX, currY;
  uint8_t currPen, flushedPen;

	DACClass *dac;
};

extern RendererClass Renderer;

#endif
