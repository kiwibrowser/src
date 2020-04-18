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

#include <exception>
#include <unwind.h>
#include "cxxabi_defines.h"
#include "helper_func_internal.h"

namespace __cxxabiv1 {

  const __shim_type_info* getTypePtr(uint64_t ttypeIndex,
                                     const uint8_t* classInfo,
                                     uint8_t ttypeEncoding,
                                     _Unwind_Exception* unwind_exception);

  _GABIXX_NORETURN void call_terminate(_Unwind_Exception* unwind_exception) {
    __cxa_begin_catch(unwind_exception);  // terminate is also a handler
    std::terminate();
  }

  // Boring stuff which has lots of encode/decode details
  void scanEHTable(ScanResultInternal& results,
                   _Unwind_Action actions,
                   bool native_exception,
                   _Unwind_Exception* unwind_exception,
                   _Unwind_Context* context) {
    // Initialize results to found nothing but an error
    results.ttypeIndex = 0;
    results.actionRecord = 0;
    results.languageSpecificData = 0;
    results.landingPad = 0;
    results.adjustedPtr = 0;
    results.reason = _URC_FATAL_PHASE1_ERROR;

    // Check for consistent actions
    if (actions & _UA_SEARCH_PHASE) {
      if (actions & (_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME | _UA_FORCE_UNWIND)) {
        results.reason = _URC_FATAL_PHASE1_ERROR;
        return;
      }
    } else if (actions & _UA_CLEANUP_PHASE) {
      if ((actions & _UA_HANDLER_FRAME) && (actions & _UA_FORCE_UNWIND)) {
        results.reason = _URC_FATAL_PHASE2_ERROR;
        return;
      }
    } else {
      results.reason = _URC_FATAL_PHASE1_ERROR;
      return;
    }


    // Start scan by getting exception table address
    const uint8_t* lsda = (const uint8_t*)_Unwind_GetLanguageSpecificData(context);
    if (lsda == 0) {
      // No exception table
      results.reason = _URC_CONTINUE_UNWIND;
      return;
    }
    results.languageSpecificData = lsda;
    uintptr_t ip = _Unwind_GetIP(context) - 1;
    uintptr_t funcStart = _Unwind_GetRegionStart(context);
    uintptr_t ipOffset = ip - funcStart;
    const uint8_t* classInfo = NULL;
    uint8_t lpStartEncoding = *lsda++;
    const uint8_t* lpStart = (const uint8_t*)readEncodedPointer(&lsda, lpStartEncoding);
    if (lpStart == 0) {
      lpStart = (const uint8_t*)funcStart;
    }
    uint8_t ttypeEncoding = *lsda++;
    if (ttypeEncoding != DW_EH_PE_omit) {
      uintptr_t classInfoOffset = readULEB128(&lsda);
      classInfo = lsda + classInfoOffset;
    }
    uint8_t callSiteEncoding = *lsda++;
    uint32_t callSiteTableLength = static_cast<uint32_t>(readULEB128(&lsda));
    const uint8_t* callSiteTableStart = lsda;
    const uint8_t* callSiteTableEnd = callSiteTableStart + callSiteTableLength;
    const uint8_t* actionTableStart = callSiteTableEnd;
    const uint8_t* callSitePtr = callSiteTableStart;


    while (callSitePtr < callSiteTableEnd) {
      uintptr_t start = readEncodedPointer(&callSitePtr, callSiteEncoding);
      uintptr_t length = readEncodedPointer(&callSitePtr, callSiteEncoding);
      uintptr_t landingPad = readEncodedPointer(&callSitePtr, callSiteEncoding);
      uintptr_t actionEntry = readULEB128(&callSitePtr);
      if ((start <= ipOffset) && (ipOffset < (start + length))) {
        if (landingPad == 0) {
          // No handler here
          results.reason = _URC_CONTINUE_UNWIND;
          return;
        }

        landingPad = (uintptr_t)lpStart + landingPad;
        if (actionEntry == 0) {
          if ((actions & _UA_CLEANUP_PHASE) && !(actions & _UA_HANDLER_FRAME))
          {
            results.ttypeIndex = 0;
            results.landingPad = landingPad;
            results.reason = _URC_HANDLER_FOUND;
            return;
          }
          // No handler here
          results.reason = _URC_CONTINUE_UNWIND;
          return;
        }

        const uint8_t* action = actionTableStart + (actionEntry - 1);
        while (true) {
          const uint8_t* actionRecord = action;
          int64_t ttypeIndex = readSLEB128(&action);
          if (ttypeIndex > 0) {
            // Found a catch, does it actually catch?
            // First check for catch (...)
            const __shim_type_info* catchType =
              getTypePtr(static_cast<uint64_t>(ttypeIndex),
                         classInfo, ttypeEncoding, unwind_exception);
            if (catchType == 0) {
              // Found catch (...) catches everything, including foreign exceptions
              if ((actions & _UA_SEARCH_PHASE) || (actions & _UA_HANDLER_FRAME))
              {
                // Save state and return _URC_HANDLER_FOUND
                results.ttypeIndex = ttypeIndex;
                results.actionRecord = actionRecord;
                results.landingPad = landingPad;
                results.adjustedPtr = unwind_exception+1;
                results.reason = _URC_HANDLER_FOUND;
                return;
              }
              else if (!(actions & _UA_FORCE_UNWIND))
              {
                // It looks like the exception table has changed
                //    on us.  Likely stack corruption!
                call_terminate(unwind_exception);
              }
            } else if (native_exception) {
              __cxa_exception* exception_header = (__cxa_exception*)(unwind_exception+1) - 1;
              void* adjustedPtr = unwind_exception+1;
              const __shim_type_info* excpType =
                  static_cast<const __shim_type_info*>(exception_header->exceptionType);
              if (adjustedPtr == 0 || excpType == 0) {
                // Such a disaster! What's wrong?
                call_terminate(unwind_exception);
              }

              // Only derefence once, so put ouside the recursive search below
              if (dynamic_cast<const __pointer_type_info*>(excpType)) {
                adjustedPtr = *static_cast<void**>(adjustedPtr);
              }

              // Let's play!
              if (catchType->can_catch(excpType, adjustedPtr)) {
                if (actions & _UA_SEARCH_PHASE) {
                  // Cache it.
                  results.ttypeIndex = ttypeIndex;
                  results.actionRecord = actionRecord;
                  results.landingPad = landingPad;
                  results.adjustedPtr = adjustedPtr;
                  results.reason = _URC_HANDLER_FOUND;
                  return;
                } else if (!(actions & _UA_FORCE_UNWIND)) {
                  // It looks like the exception table has changed
                  //    on us.  Likely stack corruption!
                  call_terminate(unwind_exception);
                }
              } // catchType->can_catch
            } // if (catchType == 0)
          } else if (ttypeIndex < 0) {
            // Found an exception spec.
            if (native_exception) {
              __cxa_exception* header = reinterpret_cast<__cxa_exception*>(unwind_exception+1)-1;
              void* adjustedPtr = unwind_exception+1;
              const std::type_info* excpType = header->exceptionType;
              if (adjustedPtr == 0 || excpType == 0) {
                // Such a disaster! What's wrong?
                call_terminate(unwind_exception);
              }

              // Let's play!
              if (canExceptionSpecCatch(ttypeIndex, classInfo,
                                        ttypeEncoding, excpType,
                                        adjustedPtr, unwind_exception)) {
                if (actions & _UA_SEARCH_PHASE) {
                  // Cache it.
                  results.ttypeIndex = ttypeIndex;
                  results.actionRecord = actionRecord;
                  results.landingPad = landingPad;
                  results.adjustedPtr = adjustedPtr;
                  results.reason = _URC_HANDLER_FOUND;
                  return;
                } else if (!(actions & _UA_FORCE_UNWIND)) {
                  // It looks like the exception table has changed
                  //    on us.  Likely stack corruption!
                  call_terminate(unwind_exception);
                }
              }
            } else {  // ! native_exception
              // foreign exception must be caught by exception spec
              if ((actions & _UA_SEARCH_PHASE) || (actions & _UA_HANDLER_FRAME)) {
                results.ttypeIndex = ttypeIndex;
                results.actionRecord = actionRecord;
                results.landingPad = landingPad;
                results.adjustedPtr = unwind_exception+1;
                results.reason = _URC_HANDLER_FOUND;
                return;
              }
              else if (!(actions & _UA_FORCE_UNWIND)) {
                // It looks like the exception table has changed
                //    on us.  Likely stack corruption!
                call_terminate(unwind_exception);
              }
            }
          } else {  // ttypeIndex == 0
            // Found a cleanup, or nothing
            if ((actions & _UA_CLEANUP_PHASE) && !(actions & _UA_HANDLER_FRAME)) {
              results.ttypeIndex = ttypeIndex;
              results.actionRecord = actionRecord;
              results.landingPad = landingPad;
              results.adjustedPtr = unwind_exception+1;
              results.reason = _URC_HANDLER_FOUND;
              return;
            }
          }


          const uint8_t* temp = action;
          int64_t actionOffset = readSLEB128(&temp);
          if (actionOffset == 0) {
            // End of action list, no matching handler or cleanup found
            results.reason = _URC_CONTINUE_UNWIND;
            return;
          }

          // Go to next action
          action += actionOffset;
        }
      } else if (ipOffset < start) {
        // There is no call site for this ip
        call_terminate(unwind_exception);
      }
    } // while (callSitePtr < callSiteTableEnd)

    call_terminate(unwind_exception);
  }

  /*
   * Below is target-dependent part
   */

#ifdef __arm__

  /* Decode an R_ARM_TARGET2 relocation.  */
  uint32_t decodeRelocTarget2 (uint32_t ptr) {
    uint32_t tmp;

    tmp = *reinterpret_cast<uint32_t*>(ptr);
    if (!tmp) {
      return 0;
    }

    tmp += ptr;
    tmp = *reinterpret_cast<uint32_t*>(tmp);
    return tmp;
  }

  const __shim_type_info* getTypePtr(uint64_t ttypeIndex,
                                     const uint8_t* classInfo,
                                     uint8_t ttypeEncoding,
                                     _Unwind_Exception* unwind_exception) {
    if (classInfo == 0) { // eh table corrupted!
      call_terminate(unwind_exception);
    }
    const uint8_t* ptr = classInfo - ttypeIndex * 4;
    return (const __shim_type_info*)decodeRelocTarget2((uint32_t)ptr);
  }

  bool canExceptionSpecCatch(int64_t specIndex,
                             const uint8_t* classInfo,
                             uint8_t ttypeEncoding,
                             const std::type_info* excpType,
                             void* adjustedPtr,
                             _Unwind_Exception* unwind_exception) {
    if (classInfo == 0) { // eh table corrupted!
      call_terminate(unwind_exception);
    }

    specIndex = -specIndex;
    specIndex -= 1;
    const uint32_t* temp = reinterpret_cast<const uint32_t*>(classInfo) + specIndex;

    while (true) {
      uint32_t ttypeIndex = *temp;
      if (ttypeIndex == 0) {
        break;
      }
      ttypeIndex = decodeRelocTarget2((uint32_t)temp);
      temp += 1;
      const __shim_type_info* catchType = (const __shim_type_info*) ttypeIndex;
      void* tempPtr = adjustedPtr;
      if (catchType->can_catch(
              static_cast<const __shim_type_info*>(excpType), tempPtr)) {
        return false;
      }
    } // while
    return true;
  }

  // lower-level runtime library API function that unwinds the frame
  extern "C" _Unwind_Reason_Code __gnu_unwind_frame(_Unwind_Exception*,
                                                    _Unwind_Context*);

  void setRegisters(_Unwind_Exception* unwind_exception,
                    _Unwind_Context* context,
                    const ScanResultInternal& results) {
    _Unwind_SetGR(context, 0, reinterpret_cast<uintptr_t>(unwind_exception));
    _Unwind_SetGR(context, 1, static_cast<uintptr_t>(results.ttypeIndex));
    _Unwind_SetIP(context, results.landingPad);
  }

  _Unwind_Reason_Code continueUnwinding(_Unwind_Exception *ex,
                                        _Unwind_Context *context) {
    if (__gnu_unwind_frame(ex, context) != _URC_OK) {
      return _URC_FAILURE;
    }
    return _URC_CONTINUE_UNWIND;
  }

  void saveDataToBarrierCache(_Unwind_Exception* exc,
                              _Unwind_Context* ctx,
                              const ScanResultInternal& results) {
    exc->barrier_cache.sp = _Unwind_GetGR(ctx, UNWIND_STACK_REG);
    exc->barrier_cache.bitpattern[0] = (uint32_t)results.adjustedPtr;
    exc->barrier_cache.bitpattern[1] = (uint32_t)results.ttypeIndex;
    exc->barrier_cache.bitpattern[3] = (uint32_t)results.landingPad;
  }

  void loadDataFromBarrierCache(_Unwind_Exception* exc,
                                ScanResultInternal& results) {
    results.adjustedPtr = (void*) exc->barrier_cache.bitpattern[0];
    results.ttypeIndex = (int64_t) exc->barrier_cache.bitpattern[1];
    results.landingPad = (uintptr_t) exc->barrier_cache.bitpattern[3];
  }

  void prepareBeginCleanup(_Unwind_Exception* exc) {
    __cxa_begin_cleanup(exc);
  }

  void saveUnexpectedDataToBarrierCache(_Unwind_Exception* exc,
                                        _Unwind_Context* ctx,
                                        const ScanResultInternal& results) {
    prepareBeginCleanup(exc);

    const uint8_t* lsda = (const uint8_t*)_Unwind_GetLanguageSpecificData(ctx);
    const uint8_t* classInfo = NULL;
    uint8_t lpStartEncoding = *lsda++;
    __attribute__((unused))
    const uint8_t* lpStart = (const uint8_t*)readEncodedPointer(&lsda, lpStartEncoding);
    __attribute__((unused))
    uintptr_t funcStart = _Unwind_GetRegionStart(ctx);
    uint8_t ttypeEncoding = *lsda++;
    if (ttypeEncoding != DW_EH_PE_omit) {
      uintptr_t classInfoOffset = readULEB128(&lsda);
      classInfo = lsda + classInfoOffset;
    }

    const uint32_t* e = (const uint32_t*) classInfo - results.ttypeIndex - 1;
    uint32_t n = 0;
    while (e[n] != 0) {
      ++n;
    }

    exc->barrier_cache.bitpattern[1] = n;
    exc->barrier_cache.bitpattern[3] = 4;
    exc->barrier_cache.bitpattern[4] = (uint32_t)e;
  }

#else // ! __arm__

  const __shim_type_info* getTypePtr(uint64_t ttypeIndex,
                                     const uint8_t* classInfo,
                                     uint8_t ttypeEncoding,
                                     _Unwind_Exception* unwind_exception) {
    if (classInfo == 0) { // eh table corrupted!
      call_terminate(unwind_exception);
    }

    switch (ttypeEncoding & 0x0F) {
    case DW_EH_PE_absptr:
      ttypeIndex *= sizeof(void*);
      break;
    case DW_EH_PE_udata2:
    case DW_EH_PE_sdata2:
      ttypeIndex *= 2;
      break;
    case DW_EH_PE_udata4:
    case DW_EH_PE_sdata4:
      ttypeIndex *= 4;
      break;
    case DW_EH_PE_udata8:
    case DW_EH_PE_sdata8:
      ttypeIndex *= 8;
      break;
    default:
      // this should not happen.
      call_terminate(unwind_exception);
    }
    classInfo -= ttypeIndex;
    return (const __shim_type_info*)readEncodedPointer(&classInfo, ttypeEncoding);
  }

  bool canExceptionSpecCatch(int64_t specIndex,
                             const uint8_t* classInfo,
                             uint8_t ttypeEncoding,
                             const std::type_info* excpType,
                             void* adjustedPtr,
                             _Unwind_Exception* unwind_exception) {
    if (classInfo == 0) { // eh table corrupted!
      call_terminate(unwind_exception);
    }

    specIndex = -specIndex;
    specIndex -= 1;
    const uint8_t* temp = classInfo + specIndex;

    while (true) {
      uint64_t ttypeIndex = readULEB128(&temp);
      if (ttypeIndex == 0) {
        break;
      }
      const __shim_type_info* catchType = getTypePtr(ttypeIndex,
                                                     classInfo,
                                                     ttypeEncoding,
                                                     unwind_exception);
      void* tempPtr = adjustedPtr;
      if (catchType->can_catch(
              static_cast<const __shim_type_info*>(excpType), tempPtr)) {
        return false;
      }
    } // while
    return true;
  }

  void setRegisters(_Unwind_Exception* unwind_exception,
                    _Unwind_Context* context,
                    const ScanResultInternal& results) {
    _Unwind_SetGR(context, __builtin_eh_return_data_regno(0),
                  reinterpret_cast<uintptr_t>(unwind_exception));
    _Unwind_SetGR(context, __builtin_eh_return_data_regno(1),
                  static_cast<uintptr_t>(results.ttypeIndex));
    _Unwind_SetIP(context, results.landingPad);
  }

  _Unwind_Reason_Code continueUnwinding(_Unwind_Exception *ex,
                                        _Unwind_Context *context) {
    return _URC_CONTINUE_UNWIND;
  }

  // Do nothing, only for API compatibility
  // We don't use C++ polymorphism since we hope no virtual table cost.
  void saveDataToBarrierCache(_Unwind_Exception* exc,
                              _Unwind_Context* ctx,
                              const ScanResultInternal& results) {}

  void loadDataFromBarrierCache(_Unwind_Exception* exc,
                                ScanResultInternal& results) {}

  void prepareBeginCleanup(_Unwind_Exception* exc) {}

  void saveUnexpectedDataToBarrierCache(_Unwind_Exception* exc,
                                        _Unwind_Context* ctx,
                                        const ScanResultInternal& results) {}

#endif // __arm__

} // namespace __cxxabiv1
