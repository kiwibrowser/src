/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file is for linking into pexes when C++ exception handling is
 * disabled.  libstdc++ expects to be able to call _Unwind_*()
 * functions, which are normally provided by libgcc_eh, which is not
 * accessible in PNaCl's stable ABI.  This file provides stubs for the
 * _Unwind_*() functions which just abort().
 */

#include <stdlib.h>
#include <unistd.h>

/*
 * unwind.h will generate static definitions of some of the functions when
 * the __arm__ macro is defined but we want to provide our own, exported
 * stubs because this will be used as portable bitcode.
 */
#undef __arm__
#include <unwind.h>

#define STUB                                                      \
  char msg1[] = "Aborting: ";                                     \
  char msg2[] = " called (C++ exception handling is disabled)\n"; \
  write(2, msg1, sizeof(msg1) - 1);                               \
  write(2, __func__, sizeof(__func__) - 1);                       \
  write(2, msg2, sizeof(msg2) - 1);                               \
  abort();

void _Unwind_DeleteException(struct _Unwind_Exception *e) {
  STUB
}

_Unwind_Ptr _Unwind_GetRegionStart(struct _Unwind_Context *c) {
  STUB
}

_Unwind_Ptr _Unwind_GetDataRelBase(struct _Unwind_Context *c) {
  STUB
}

_Unwind_Word _Unwind_GetGR(struct _Unwind_Context *c, int i) {
  STUB
}

_Unwind_Ptr _Unwind_GetIP(struct _Unwind_Context *c) {
  STUB
}

_Unwind_Ptr _Unwind_GetIPInfo(struct _Unwind_Context *c, int *i) {
  STUB
}

_Unwind_Ptr _Unwind_GetTextRelBase(struct _Unwind_Context *c) {
  STUB
}

void *_Unwind_GetLanguageSpecificData(struct _Unwind_Context *c) {
  STUB
}

void _Unwind_SetGR(struct _Unwind_Context *c, int i, _Unwind_Word w) {
  STUB
}

void _Unwind_SetIP(struct _Unwind_Context *c, _Unwind_Ptr p) {
  STUB
}

void _Unwind_PNaClSetResult0(struct _Unwind_Context *c, _Unwind_Word w) {
  STUB
}

void _Unwind_PNaClSetResult1(struct _Unwind_Context *c, _Unwind_Word w) {
  STUB
}

_Unwind_Reason_Code _Unwind_RaiseException(struct _Unwind_Exception *e) {
  STUB
}

_Unwind_Reason_Code _Unwind_Resume_or_Rethrow(struct _Unwind_Exception *e) {
  STUB
}

void _Unwind_Resume(struct _Unwind_Exception *e) {
  STUB
}

_Unwind_Word _Unwind_GetCFA(struct _Unwind_Context *c) {
  STUB
}

_Unwind_Reason_Code _Unwind_Backtrace(_Unwind_Trace_Fn fn, void *p) {
  STUB
}
