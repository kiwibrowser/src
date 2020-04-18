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

#ifndef __GABIXX_UNWIND_ARM_H__
#define __GABIXX_UNWIND_ARM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8,
  _URC_FAILURE = 9,
  _URC_OK = 0
} _Unwind_Reason_Code;

typedef uint32_t _Unwind_State;
static const _Unwind_State _US_VIRTUAL_UNWIND_FRAME   = 0;
static const _Unwind_State _US_UNWIND_FRAME_STARTING  = 1;
static const _Unwind_State _US_UNWIND_FRAME_RESUME    = 2;

typedef struct _Unwind_Control_Block _Unwind_Control_Block;
typedef struct _Unwind_Context _Unwind_Context;
typedef uint32_t _Unwind_EHT_Header;

struct _Unwind_Control_Block {
  uint64_t exception_class; // Compatible with Itanium ABI
  void (*exception_cleanup)(_Unwind_Reason_Code, _Unwind_Control_Block*);

  struct {
    uint32_t reserved1; // init reserved1 to 0, then don't touch
    uint32_t reserved2;
    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t reserved5;
  } unwinder_cache;

  struct {
    uint32_t sp;
    uint32_t bitpattern[5];
  } barrier_cache;

  struct {
    uint32_t bitpattern[4];
  } cleanup_cache;

  struct {
    uint32_t fnstart; // function start address
    _Unwind_EHT_Header* ehtp; // pointer to EHT entry header word
    uint32_t additional;  // additional data
    uint32_t reserved1;
  } pr_cache;

  long long int : 0;  // Force alignment of next item to 8-byte boundary
};

// This makes our code more simple
typedef _Unwind_Control_Block _Unwind_Exception;

typedef enum {
  _UVRSC_CORE = 0,  // integer register
  _UVRSC_VFP = 1, // vfp
  _UVRSC_WMMXD = 3, // Intel WMMX data register
  _UVRSC_WMMXC = 4  // Intel WMMX control register
} _Unwind_VRS_RegClass;

typedef enum {
  _UVRSD_UINT32 = 0,
  _UVRSD_VFPX = 1,
  _UVRSD_UINT64 = 3,
  _UVRSD_FLOAT = 4,
  _UVRSD_DOUBLE = 5
} _Unwind_VRS_DataRepresentation;

typedef enum {
  _UVRSR_OK = 0,
  _UVRSR_NOT_IMPLEMENTED = 1,
  _UVRSR_FAILED = 2
} _Unwind_VRS_Result;

_Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception* ucbp);
void _Unwind_Resume(_Unwind_Exception* ucbp);
void _Unwind_Complete(_Unwind_Exception* ucbp);
void _Unwind_DeleteException(_Unwind_Exception* ucbp);
uint64_t _Unwind_GetRegionStart(_Unwind_Context*);
void* _Unwind_GetLanguageSpecificData(_Unwind_Context*);


_Unwind_VRS_Result _Unwind_VRS_Get(_Unwind_Context *context,
                                   _Unwind_VRS_RegClass regclass,
                                   uint32_t regno,
                                   _Unwind_VRS_DataRepresentation representation,
                                   void* valuep);

_Unwind_VRS_Result _Unwind_VRS_Set(_Unwind_Context *context,
                                   _Unwind_VRS_RegClass regclass,
                                   uint32_t regno,
                                   _Unwind_VRS_DataRepresentation representation,
                                   void* valuep);

/*
 * Implement Itanium ABI based on ARM EHABI to simplify code
 */
typedef int _Unwind_Action;
static const _Unwind_Action _UA_SEARCH_PHASE  = 1;
static const _Unwind_Action _UA_CLEANUP_PHASE = 2;
static const _Unwind_Action _UA_HANDLER_FRAME = 4;
static const _Unwind_Action _UA_FORCE_UNWIND  = 8;

#define UNWIND_POINTER_REG  12
#define UNWIND_STACK_REG    13
#define UNWIND_IP_REG       15

static inline uint32_t _Unwind_GetGR( _Unwind_Context* ctx, int reg) {
  uint32_t val;
  _Unwind_VRS_Get(ctx, _UVRSC_CORE, reg, _UVRSD_UINT32, &val);
  return val;
}

static inline void _Unwind_SetGR(_Unwind_Context* ctx, int reg, uint32_t val) {
  _Unwind_VRS_Set(ctx, _UVRSC_CORE, reg, _UVRSD_UINT32, &val);
}

static inline uint32_t _Unwind_GetIP(_Unwind_Context* ctx) {
  return _Unwind_GetGR(ctx, UNWIND_IP_REG) & ~1; // thumb bit
}
static inline void _Unwind_SetIP(_Unwind_Context* ctx, uint32_t val) {
  // Propagate thumb bit to instruction pointer
  uint32_t thumbState = _Unwind_GetGR(ctx, UNWIND_IP_REG) & 1;
  _Unwind_SetGR(ctx, UNWIND_IP_REG, (val | thumbState));
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // __GABIXX_UNWIND_ARM_H__
