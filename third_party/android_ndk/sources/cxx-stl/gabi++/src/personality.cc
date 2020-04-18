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
//
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

#include <cassert>
#include <cstdlib>
#include <exception>
#include <unwind.h>
#include <typeinfo>
#include "cxxabi_defines.h"
#include "dwarf_helper.h"
#include "helper_func_internal.h"


namespace __cxxabiv1 {

  /*
   * Personality Routine
   */
  extern "C" _Unwind_Reason_Code BEGIN_DEFINE_PERSONALITY_FUNC
    if (version != 1 || unwind_exception == 0 || context == 0) {
      return _URC_FATAL_PHASE1_ERROR;
    }

    bool native_exception = exceptionClass == __gxx_exception_class;
    ScanResultInternal results;

    /*
     * Phase 1: Search
     */
    if (actions & _UA_SEARCH_PHASE) {
      scanEHTable(results, actions, native_exception, unwind_exception, context);
      if (results.reason == _URC_HANDLER_FOUND) {
        // Found one.  Cache result for phase 2.
        if (native_exception) {
          __cxa_exception* exception_header = reinterpret_cast<__cxa_exception*>(unwind_exception+1)-1;
          exception_header->handlerSwitchValue = static_cast<int>(results.ttypeIndex);
          exception_header->actionRecord = results.actionRecord;
          exception_header->languageSpecificData = results.languageSpecificData;
          exception_header->catchTemp = reinterpret_cast<void*>(results.landingPad);
          exception_header->adjustedPtr = results.adjustedPtr;
          saveDataToBarrierCache(unwind_exception, context, results);
        }
        return _URC_HANDLER_FOUND;
      }
      return continueUnwinding(unwind_exception, context);
    }

    /*
     * Phase 2: Cleanup
     */
    if (actions & _UA_CLEANUP_PHASE) {
      if (actions & _UA_HANDLER_FRAME) {
        if (native_exception) {
          // We have cached on phase 1. Just recall that.
          __cxa_exception* exception_header = reinterpret_cast<__cxa_exception*>(unwind_exception+1)-1;
          results.ttypeIndex = exception_header->handlerSwitchValue;
          results.actionRecord = exception_header->actionRecord;
          results.languageSpecificData = exception_header->languageSpecificData;
          results.landingPad = reinterpret_cast<uintptr_t>(exception_header->catchTemp);
          results.adjustedPtr = exception_header->adjustedPtr;
          loadDataFromBarrierCache(unwind_exception, results);
        } else {
          scanEHTable(results, actions, native_exception, unwind_exception, context);

          if (results.reason != _URC_HANDLER_FOUND)
            call_terminate(unwind_exception);
        }

        setRegisters(unwind_exception, context, results);
        saveUnexpectedDataToBarrierCache(unwind_exception,
                                         context,
                                         results);
        return _URC_INSTALL_CONTEXT;
      }

      // Force unwinding or no handler found
      scanEHTable(results, actions, native_exception, unwind_exception, context);
      if (results.reason == _URC_HANDLER_FOUND) {
        // Found a non-catching handler.  Jump to it.
        setRegisters(unwind_exception, context, results);
        prepareBeginCleanup(unwind_exception);
        return _URC_INSTALL_CONTEXT;
      }
      return continueUnwinding(unwind_exception, context);
    }

    // Why here? Neither phase 1 nor 2.
    return _URC_FATAL_PHASE1_ERROR;
  }

}  // namespace __cxxabiv1
