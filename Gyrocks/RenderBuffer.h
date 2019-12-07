/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef RENDER_BUFFER_H
#define RENDER_BUFFER_H

#include "Arduino.h"

#define CMD_PE  0x01
#define CMD_PA  0x02

class RenderBufferClass {
public:
  void begin(uint32_t _bufferSize);
  void end();

  void nf(uint8_t D);
  void pe(uint8_t D);
  void pa(uint16_t X, uint16_t Y);

  void render(uint8_t D = 0);

private:
  static const int MAX_FILES = 32;

  struct FileStruct {
    uint32_t *pFileData;
    uint32_t fileSize;
  } files[MAX_FILES];
  
  uint32_t bufferSize;
  uint32_t bufferCounter;
  uint32_t *buffer;
  
  FileStruct *pCurrFile;
};

extern RenderBufferClass RenderBuffer;

#endif
