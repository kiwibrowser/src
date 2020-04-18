// Copyright (C) 2012 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#ifndef __GABIXX_UNWIND_ITANIUM_H__
#define __GABIXX_UNWIND_ITANIUM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8
} _Unwind_Reason_Code;

typedef int _Unwind_Action;
static const _Unwind_Action _UA_SEARCH_PHASE  = 1;
static const _Unwind_Action _UA_CLEANUP_PHASE = 2;
static const _Unwind_Action _UA_HANDLER_FRAME = 4;
static const _Unwind_Action _UA_FORCE_UNWIND  = 8;

struct _Unwind_Context; // system-specific opaque
struct _Unwind_Exception;

typedef void (*_Unwind_Exception_Cleanup_Fn) (_Unwind_Reason_Code reason,
                                              struct _Unwind_Exception* exc);

typedef _Unwind_Reason_Code (*_Unwind_Stop_Fn) (int version,
                                                _Unwind_Action actions,
                                                uint64_t exceptionClass,
                                                struct _Unwind_Exception*,
                                                struct _Unwind_Context*,
                                                void* stop_parameter);

struct _Unwind_Exception {
  uint64_t exception_class;
  _Unwind_Exception_Cleanup_Fn exception_cleanup;

  /**
   * The architectures supported by the following declarations are:
   *  x86 with LP32, x86_64 with LP64
   *  arm64 with LP64
   *  mips, mips64
   */
  unsigned long private_1;
  unsigned long private_2;
}
#if defined(__clang__) && defined(__mips__)
// FIXME: It seems that mipsel-linux-android-gcc will use 24 as the object size
// with or without the aligned attribute.  However, clang (mipsel) will align
// the object size to 32 when we specify the aligned attribute, which may
// result in some sort of incompatibility.  As a workaround, let's remove this
// attribute when we are compiling this file for MIPS architecture with clang.
// Add the attribute back when clang can have same behavior as gcc.
#else
__attribute__((__aligned__)) // must be double-word aligned
#endif
;

_Unwind_Reason_Code _Unwind_RaiseException(struct _Unwind_Exception*);
void _Unwind_Resume(struct _Unwind_Exception*);
void _Unwind_DeleteException(struct _Unwind_Exception*);

uint64_t _Unwind_GetGR(struct _Unwind_Context*, int index);
void _Unwind_SetGR(struct _Unwind_Context*, int index, uint64_t new_value);

uint64_t _Unwind_GetIP(struct _Unwind_Context*);
void _Unwind_SetIP(struct _Unwind_Context*, uintptr_t new_value);

uint64_t _Unwind_GetRegionStart(struct _Unwind_Context*);
uint64_t _Unwind_GetLanguageSpecificData(struct _Unwind_Context*);

_Unwind_Reason_Code _Unwind_ForcedUnwind(struct _Unwind_Exception*,
                                         _Unwind_Stop_Fn stop,
                                         void* stop_parameter);

_Unwind_Reason_Code _Unwind_RaiseException(struct _Unwind_Exception*);
void _Unwind_Resume(struct _Unwind_Exception*);
void _Unwind_DeleteException(struct _Unwind_Exception*);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // __GABIXX_UNWIND_ITANIUM_H__
