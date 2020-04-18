// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/stability_debugging.h"

#include <windows.h>

#include <memory>

#include "base/debug/activity_tracker.h"
#include "build/build_config.h"

namespace browser_watcher {

namespace {

struct VehUnregisterer {
  void operator()(void* handle) const {
    ::RemoveVectoredExceptionHandler(handle);
  }
};

using VehHandle = std::unique_ptr<void, VehUnregisterer>;

uintptr_t GetProgramCounter(const CONTEXT& context) {
#if defined(ARCH_CPU_X86)
  return context.Eip;
#elif defined(ARCH_CPU_X86_64)
  return context.Rip;
#endif
}

LONG CALLBACK VectoredExceptionHandler(EXCEPTION_POINTERS* exception_pointers) {
  base::debug::GlobalActivityTracker* tracker =
      base::debug::GlobalActivityTracker::Get();
  if (tracker) {
    EXCEPTION_RECORD* record = exception_pointers->ExceptionRecord;
    uintptr_t pc = GetProgramCounter(*exception_pointers->ContextRecord);
    tracker->RecordException(reinterpret_cast<void*>(pc),
                             record->ExceptionAddress, record->ExceptionCode);
  }

  return EXCEPTION_CONTINUE_SEARCH;  // Continue to the next handler.
}

}  // namespace

void SetStabilityDataBool(base::StringPiece name, bool value) {
  base::debug::GlobalActivityTracker* global_tracker =
      base::debug::GlobalActivityTracker::Get();
  if (!global_tracker)
    return;  // Activity tracking isn't enabled.

  global_tracker->process_data().SetBool(name, value);
}

void SetStabilityDataInt(base::StringPiece name, int64_t value) {
  base::debug::GlobalActivityTracker* global_tracker =
      base::debug::GlobalActivityTracker::Get();
  if (!global_tracker)
    return;  // Activity tracking isn't enabled.

  global_tracker->process_data().SetInt(name, value);
}

void RegisterStabilityVEH() {
#if defined(ADDRESS_SANITIZER)
  // ASAN on windows x64 is dynamically allocating the shadow memory on a
  // memory access violation by setting up an vector exception handler.
  // When instrumented with ASAN, this code may trigger an exception by
  // accessing unallocated shadow memory, which is causing an infinite
  // recursion (i.e. infinite memory access violation).
  (void)&VectoredExceptionHandler;
#else
  // Register a vectored exception handler and request it be first. Note that
  // subsequent registrations may also request to be first, in which case this
  // one will be bumped.
  // TODO(manzagop): Depending on observations, it may be necessary to
  // consider refreshing the registration, either periodically or at opportune
  // (e.g. risky) times.
  static VehHandle veh_handler(
      ::AddVectoredExceptionHandler(1, &VectoredExceptionHandler));
  DCHECK(veh_handler);
#endif  // ADDRESS_SANITIZER
}

}  // namespace browser_watcher
