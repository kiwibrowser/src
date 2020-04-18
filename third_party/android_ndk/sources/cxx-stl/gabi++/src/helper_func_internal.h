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

#ifndef __GXXABI_HELPER_FUNC_INTERNAL_H
#define __GXXABI_HELPER_FUNC_INTERNAL_H

#include <cxxabi.h>
#include <exception>
#include <unwind.h>
#include "dwarf_helper.h"

// Target-independent helper functions
namespace __cxxabiv1 {

  _GABIXX_NORETURN void call_terminate(_Unwind_Exception* unwind_exception) _GABIXX_HIDDEN;

#if __arm__
  uint32_t decodeRelocTarget2 (uint32_t ptr) _GABIXX_HIDDEN;
#endif

  // An exception spec acts like a catch handler, but in reverse.
  // If any catchType in the list can catch an excpType,
  // then this exception spec does not catch the excpType.
  bool canExceptionSpecCatch(int64_t specIndex,
                             const uint8_t* classInfo,
                             uint8_t ttypeEncoding,
                             const std::type_info* excpType,
                             void* adjustedPtr,
                             _Unwind_Exception* unwind_exception)
      _GABIXX_HIDDEN;

  void setRegisters(_Unwind_Exception* unwind_exception,
                    _Unwind_Context* context,
                    const ScanResultInternal& results) _GABIXX_HIDDEN;

  _Unwind_Reason_Code continueUnwinding(_Unwind_Exception *ex,
                                        _Unwind_Context *context)
      _GABIXX_HIDDEN;

  void saveDataToBarrierCache(_Unwind_Exception* exc,
                              _Unwind_Context* ctx,
                              const ScanResultInternal& results)
      _GABIXX_HIDDEN;

  void loadDataFromBarrierCache(_Unwind_Exception* exc,
                                ScanResultInternal& results)
      _GABIXX_HIDDEN;

  void prepareBeginCleanup(_Unwind_Exception* exc) _GABIXX_HIDDEN;

  void saveUnexpectedDataToBarrierCache(_Unwind_Exception* exc,
                                        _Unwind_Context* ctx,
                                        const ScanResultInternal& results)
      _GABIXX_HIDDEN;

  void scanEHTable(ScanResultInternal& results,
                   _Unwind_Action actions,
                   bool native_exception,
                   _Unwind_Exception* unwind_exception,
                   _Unwind_Context* context) _GABIXX_HIDDEN;

  // Make it easier to adapt to Itanium PR
#ifdef __arm__

  extern "C"
  _Unwind_Reason_Code __gxx_personality_v0(_Unwind_State,
                                           _Unwind_Exception*,
                                           _Unwind_Context*) _GABIXX_DEFAULT;

#  define BEGIN_DEFINE_PERSONALITY_FUNC \
    __gxx_personality_v0(_Unwind_State state, \
                         _Unwind_Exception* unwind_exception, \
                         _Unwind_Context* context) { \
      int version = 1; \
      uint64_t exceptionClass = unwind_exception->exception_class; \
      _Unwind_Action actions = 0; \
      switch (state) { \
      default: { \
        return _URC_FAILURE; \
      } \
      case _US_VIRTUAL_UNWIND_FRAME: { \
        actions = _UA_SEARCH_PHASE; \
        break; \
      } \
      case _US_UNWIND_FRAME_STARTING: { \
        actions = _UA_CLEANUP_PHASE; \
        if (unwind_exception->barrier_cache.sp == _Unwind_GetGR(context, UNWIND_STACK_REG)) { \
          actions |= _UA_HANDLER_FRAME; \
        } \
        break; \
      } \
      case _US_UNWIND_FRAME_RESUME: { \
        return continueUnwinding(unwind_exception, context); \
        break; \
      } \
      } \
      _Unwind_SetGR (context, UNWIND_POINTER_REG, reinterpret_cast<uint32_t>(unwind_exception));
#else // ! __arm__

  extern "C"
  _Unwind_Reason_Code __gxx_personality_v0(int, _Unwind_Action, uint64_t,
                                           _Unwind_Exception*,
                                           _Unwind_Context*) _GABIXX_DEFAULT;

#  define BEGIN_DEFINE_PERSONALITY_FUNC \
      __gxx_personality_v0(int version, _Unwind_Action actions, uint64_t exceptionClass, \
                           _Unwind_Exception* unwind_exception, _Unwind_Context* context) {
#endif

} // namespace __cxxabiv1

#endif // __GXXABI_HELPER_FUNC_INTERNAL_H
