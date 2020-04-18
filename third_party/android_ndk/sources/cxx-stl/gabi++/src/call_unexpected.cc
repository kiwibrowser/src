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
//===----------------------------------------------------------------------===//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//
//  This file implements the "Exception Handling APIs"
//  http://www.codesourcery.com/public/cxx-abi/abi-eh.html
//  http://www.intel.com/design/itanium/downloads/245358.htm
//
//===----------------------------------------------------------------------===//
/*
 * Copyright 2010-2011 PathScale, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <cstdlib>
#include <unwind.h>

#include "cxxabi_defines.h"
#include "dwarf_helper.h"
#include "helper_func_internal.h"

namespace __cxxabiv1 {

#ifdef __arm__
extern "C" enum type_match_result {
  ctm_failed = 0,
  ctm_succeeded = 1,
  ctm_succeeded_with_ptr_to_base = 2
};


extern "C" type_match_result __attribute__((visibility("default")))
__cxa_type_match(_Unwind_Exception* ucbp,
                 const __shim_type_info* rttip,
                 bool is_reference_type,
                 void** matched_object) {

  __cxa_exception* header = reinterpret_cast<__cxa_exception*>(ucbp+1)-1;
  type_match_result result = ctm_succeeded;

  void* adjustedPtr = header+1;
  if (dynamic_cast<const __pointer_type_info*>(header->exceptionType)) {
    adjustedPtr = *reinterpret_cast<void**>(adjustedPtr);
    result = ctm_succeeded_with_ptr_to_base;
  }

  const __shim_type_info* catch_type = rttip;
  const __shim_type_info* thrown_type =
      static_cast<const __shim_type_info*>(header->exceptionType);
  if (!catch_type || !thrown_type) {
    return ctm_failed;
  }

  if (catch_type->can_catch(thrown_type, adjustedPtr)) {
    *matched_object = adjustedPtr;
    return result;
  }

  return ctm_failed;
}
#endif  // __arm__

namespace {

void terminate_helper(std::terminate_handler t_handler) {
  try {
    t_handler();
    abort();
  } catch (...) {
    abort();
  }
}

void unexpected_helper(std::unexpected_handler u_handler) {
  u_handler();
  std::terminate();
}

}  // namespace

#ifdef __arm__
  extern "C" bool   __attribute__((visibility("default")))
  __cxa_begin_cleanup(_Unwind_Exception* exc) {
    __cxa_eh_globals *globals = __cxa_get_globals();
    __cxa_exception *header = reinterpret_cast<__cxa_exception*>(exc+1)-1;
    bool native = header->unwindHeader.exception_class == __gxx_exception_class;

    if (native) {
      header->cleanupCount += 1;
      if (header->cleanupCount == 1) {  // First time
        header->nextCleanup = globals->cleanupExceptions;
        globals->cleanupExceptions = header;
      }
    } else {
      globals->cleanupExceptions = header;
    }

    return true;
  }

  extern "C" _Unwind_Exception * helper_end_cleanup() {
    __cxa_eh_globals *globals = __cxa_get_globals();
    __cxa_exception* header = globals->cleanupExceptions;

    if (!header) {
      std::terminate();
    }

    if (header->unwindHeader.exception_class == __gxx_exception_class) {
      header->cleanupCount -= 1;
      if (header->cleanupCount == 0) {  // Last one
        globals->cleanupExceptions = header->nextCleanup;
        header->nextCleanup = NULL;
      }
    } else {
      globals->cleanupExceptions = NULL;
    }

    return &header->unwindHeader;
  }

  asm (
  ".pushsection .text.__cxa_end_cleanup    \n"
  ".global __cxa_end_cleanup               \n"
  ".type __cxa_end_cleanup, \"function\"   \n"
  "__cxa_end_cleanup:                      \n"
  " push\t{r1, r2, r3, r4}                 \n"
  " bl helper_end_cleanup                  \n"
  " pop\t{r1, r2, r3, r4}                  \n"
  " bl _Unwind_Resume                      \n"
  " bl abort                               \n"
  ".popsection                             \n"
  );

  extern "C" void __attribute__((visibility("default")))
  __cxa_call_unexpected(void* arg) {
    _Unwind_Exception* unwind_exception = static_cast<_Unwind_Exception*>(arg);
    __cxa_exception* header = reinterpret_cast<__cxa_exception*>(unwind_exception+1)-1;
    bool native_exception = unwind_exception->exception_class == __gxx_exception_class;

    if (!native_exception) {
      __cxa_begin_catch(unwind_exception);    // unexpected is also a handler
      try {
        std::unexpected();
      } catch (...) {
        std::terminate();
      }

      return;
    }

    // Cache previous data first since we will change contents below.
    uint32_t count = unwind_exception->barrier_cache.bitpattern[1];
    uint32_t stride = unwind_exception->barrier_cache.bitpattern[3];
    uint32_t* list = reinterpret_cast<uint32_t*>(
                            unwind_exception->barrier_cache.bitpattern[4]);

    __cxa_begin_catch(unwind_exception);    // unexpected is also a handler
    try {
      unexpected_helper(header->unexpectedHandler);
    } catch (...) {
      // A new exception thrown when calling unexpected.
      bool allow_bad_exception = false;

      for (uint32_t i = 0; i != count; ++i) {
        uint32_t offset = reinterpret_cast<uint32_t>(&list[i * (stride >> 2)]);
        offset = decodeRelocTarget2(offset);
        const __shim_type_info* catch_type = reinterpret_cast<const __shim_type_info*>(offset);

        __cxa_exception* new_header = __cxa_get_globals()->caughtExceptions;
        void* adjustedPtr = new_header + 1;
        if (__cxa_type_match(&new_header->unwindHeader,
                             catch_type,
                             false/* is_ref_type */,
                             &adjustedPtr) != ctm_failed) {
          throw;
        }

        void* null_adjustedPtr = NULL;
        const __shim_type_info* bad_excp =
            static_cast<const __shim_type_info*>(&typeid(std::bad_exception));
        if (catch_type->can_catch(bad_excp, null_adjustedPtr)) {
          allow_bad_exception = true;
        }
      }

      // If no other ones match, throw bad_exception.
      if (allow_bad_exception) {
        __cxa_end_catch();
        __cxa_end_catch();
        throw std::bad_exception();
      }

      terminate_helper(header->terminateHandler);
    }
  }
#else // ! __arm__
  extern "C" void __attribute__((visibility("default")))
  __cxa_call_unexpected(void* arg) {
    _Unwind_Exception* unwind_exception = static_cast<_Unwind_Exception*>(arg);
    if (unwind_exception == 0) {
      call_terminate(unwind_exception);
    }
    __cxa_begin_catch(unwind_exception);    // unexpected is also a handler

    bool native_old_exception = unwind_exception->exception_class == __gxx_exception_class;
    std::unexpected_handler u_handler;
    std::terminate_handler t_handler;
    __cxa_exception* old_exception_header = 0;
    int64_t ttypeIndex;
    const uint8_t* lsda;
    if (native_old_exception) {
      old_exception_header = reinterpret_cast<__cxa_exception*>(unwind_exception+1)-1;
      t_handler = old_exception_header->terminateHandler;
      u_handler = old_exception_header->unexpectedHandler;
      // If unexpected_helper(u_handler) rethrows the same exception,
      //   these values get overwritten by the rethrow.  So save them now:
      ttypeIndex = old_exception_header->handlerSwitchValue;
      lsda = old_exception_header->languageSpecificData;
    } else {
      t_handler = std::get_terminate();
      u_handler = std::get_unexpected();
    }

    try {
      unexpected_helper(u_handler);
    } catch (...) {
      // A new exception thrown when calling unexpected.

      if (!native_old_exception) {
        std::terminate();
      }
      uint8_t lpStartEncoding = *lsda++;
      readEncodedPointer(&lsda, lpStartEncoding);
      uint8_t ttypeEncoding = *lsda++;
      if (ttypeEncoding == DW_EH_PE_omit) {
        terminate_helper(t_handler);
      }
      uintptr_t classInfoOffset = readULEB128(&lsda);
      const uint8_t* classInfo = lsda + classInfoOffset;
      __cxa_eh_globals* globals = __cxa_get_globals_fast();
      __cxa_exception* new_exception_header = globals->caughtExceptions;
      if (new_exception_header == 0) {  // This shouldn't be able to happen!
        terminate_helper(t_handler);
      }
      bool native_new_exception =
        new_exception_header->unwindHeader.exception_class == __gxx_exception_class;

      if (native_new_exception && (new_exception_header != old_exception_header)) {
        const std::type_info* excpType = new_exception_header->exceptionType;
        if (!canExceptionSpecCatch(ttypeIndex, classInfo, ttypeEncoding,
                                   excpType, new_exception_header+1, unwind_exception)) {
          // We need to __cxa_end_catch, but for the old exception,
          //   not the new one.  This is a little tricky ...
          // Disguise new_exception_header as a rethrown exception, but
          //   don't actually rethrow it.  This means you can temporarily
          //   end the catch clause enclosing new_exception_header without
          //   __cxa_end_catch destroying new_exception_header.
          new_exception_header->handlerCount = -new_exception_header->handlerCount;
          globals->uncaughtExceptions += 1;
          __cxa_end_catch();
          __cxa_end_catch();
          __cxa_begin_catch(&new_exception_header->unwindHeader);
          throw;
        }
      }

      const std::type_info* excpType = &typeid(std::bad_exception);
      if (!canExceptionSpecCatch(ttypeIndex, classInfo, ttypeEncoding,
                                 excpType, NULL, unwind_exception)) {
        __cxa_end_catch();
        __cxa_end_catch();
        throw std::bad_exception();
      }
    } // catch (...)

    // Call terminate after unexpected normally done
    terminate_helper(t_handler);
  }
#endif // __arm__

} // namespace __cxxabiv1
