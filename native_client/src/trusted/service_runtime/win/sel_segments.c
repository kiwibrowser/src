/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Runtime.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"

#ifndef _WIN64
uint16_t NaClGetCs(void) {
  uint16_t seg1;

  __asm mov seg1, cs;
  return seg1;
}

/* there is no SetCS -- this is done via far jumps/calls */

uint16_t NaClGetDs(void) {
  uint16_t seg1;

  __asm mov seg1, ds;
  return seg1;
}


void NaClSetDs(uint16_t  seg1) {
  __asm mov ds, seg1;
}


uint16_t NaClGetEs(void) {
  uint16_t seg1;

  __asm mov seg1, es;
  return seg1;
}


void NaClSetEs(uint16_t  seg1) {
  __asm mov es, seg1;
}


uint16_t NaClGetFs(void) {
  uint16_t seg1;

  __asm mov seg1, fs;
  return seg1;
}


void NaClSetFs(uint16_t  seg1) {
  __asm mov fs, seg1;
}


uint16_t NaClGetGs(void) {
  uint16_t seg1;

  __asm mov seg1, gs;
  return seg1;
}


void NaClSetGs(uint16_t seg1) {
  __asm mov gs, seg1;
}


uint16_t NaClGetSs(void) {
  uint16_t seg1;

  __asm mov seg1, ss;
  return seg1;
}


uint32_t NaClGetStackPtr(void) {
  uint32_t stack_ptr;

  _asm mov stack_ptr, esp;
  return stack_ptr;
}
#endif
