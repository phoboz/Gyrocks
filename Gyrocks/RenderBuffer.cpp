/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "Renderer.h"
#include "RenderBuffer.h"

void RenderBufferClass::begin(uint32_t _bufferSize) {
  if (_bufferSize < 1024) {
    bufferSize = 1024;
  }
  bufferSize = _bufferSize;
  buffer = (uint32_t *) malloc(bufferSize * sizeof(uint32_t));

  bufferCounter = 0;
  
  for (int i = 0; i < MAX_FILES; i++) {
    files[i].fileSize = 0;
  }

  nf(0);
}

void RenderBufferClass::end() {
  free(buffer);
}

void RenderBufferClass::nf(uint8_t D) {
  pCurrFile = &files[D];
  pCurrFile->pFileData = &buffer[bufferCounter];
  pCurrFile->fileSize = 0;
}

void RenderBufferClass::pe(uint8_t D) {
  uint32_t elmt;
  
  if (bufferCounter <= bufferSize) {
    elmt = CMD_PE << 24;
    elmt |= D << 12;
    elmt |= 0x000;
    pCurrFile->pFileData[pCurrFile->fileSize++] = elmt;
    bufferCounter++;
  }
}

void RenderBufferClass::pa(uint16_t X, uint16_t Y) {
  uint32_t elmt;
  
  if (bufferCounter <= bufferSize) {
    elmt = CMD_PA << 24;
    elmt |= (X & 0xFFF) << 12;
    elmt |= (Y & 0xFFF);
    pCurrFile->pFileData[pCurrFile->fileSize++] = elmt;
    bufferCounter++;
  }
}

void RenderBufferClass::render(uint8_t D) {
  uint32_t *pBuf;
  uint32_t fileSize;
  uint32_t elmt;
  uint8_t cmd;
  uint16_t a, b;

  if (D == 0) {
    pBuf = buffer;
    fileSize = bufferCounter;
  }
  else if (D < MAX_FILES) {
    pBuf = files[D].pFileData;
    fileSize = files[D].fileSize;
  }
  else {
    return;
  }
  
  for (int i = 0; i < fileSize; i++) {
    elmt = pBuf[i];
    cmd = (elmt >> 24) & 0x0F;

    switch (cmd) {
      case CMD_PE:
        a = (elmt >> 12) & 0xFF;
        Renderer.pen_enable((uint8_t) a);
        break;

      case CMD_PA:
        a = (elmt >> 12) & 0xFFF;
        b = elmt & 0xFFF;
        Renderer.plot_absolute(a, b);
        break;

      default:
        break;
    }
  }
}

RenderBufferClass RenderBuffer;
