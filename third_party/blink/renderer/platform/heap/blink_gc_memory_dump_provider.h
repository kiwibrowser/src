// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_BLINK_GC_MEMORY_DUMP_PROVIDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_BLINK_GC_MEMORY_DUMP_PROVIDER_H_

#include "base/trace_event/memory_dump_provider.h"
#include "third_party/blink/renderer/platform/heap/blink_gc.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace base {
namespace trace_event {

class MemoryAllocatorDump;

}  // namespace trace_event
}  // namespace base

namespace blink {
class WebMemoryAllocatorDump;

class PLATFORM_EXPORT BlinkGCMemoryDumpProvider final
    : public base::trace_event::MemoryDumpProvider {
  USING_FAST_MALLOC(BlinkGCMemoryDumpProvider);

 public:
  static BlinkGCMemoryDumpProvider* Instance();
  ~BlinkGCMemoryDumpProvider() override;

  // MemoryDumpProvider implementation.
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs&,
                    base::trace_event::ProcessMemoryDump*) override;

  // The returned WebMemoryAllocatorDump is owned by
  // BlinkGCMemoryDumpProvider, and should not be retained (just used to
  // dump in the current call stack).
  base::trace_event::MemoryAllocatorDump* CreateMemoryAllocatorDumpForCurrentGC(
      const String& absolute_name);

  // This must be called before taking a new process-wide GC snapshot, to
  // clear the previous dumps.
  void ClearProcessDumpForCurrentGC();

  base::trace_event::ProcessMemoryDump* CurrentProcessMemoryDump() {
    return current_process_memory_dump_.get();
  }

 private:
  BlinkGCMemoryDumpProvider();

  std::unique_ptr<base::trace_event::ProcessMemoryDump>
      current_process_memory_dump_;
};

}  // namespace blink

#endif
