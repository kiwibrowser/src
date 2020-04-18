// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/blink_gc_memory_dump_provider.h"

#include <unordered_map>

#include "base/trace_event/memory_allocator_dump.h"
#include "base/trace_event/process_memory_dump.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/web_memory_allocator_dump.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace blink {
namespace {

void DumpMemoryTotals(base::trace_event::ProcessMemoryDump* memory_dump) {
  base::trace_event::MemoryAllocatorDump* allocator_dump =
      memory_dump->CreateAllocatorDump("blink_gc");
  allocator_dump->AddScalar("size", "bytes",
                            ProcessHeap::TotalAllocatedSpace());

  base::trace_event::MemoryAllocatorDump* objects_dump =
      memory_dump->CreateAllocatorDump("blink_gc/allocated_objects");

  // ThreadHeap::markedObjectSize() can be underestimated if we're still in the
  // process of lazy sweeping.
  objects_dump->AddScalar("size", "bytes",
                          ProcessHeap::TotalAllocatedObjectSize() +
                              ProcessHeap::TotalMarkedObjectSize());
}

}  // namespace

BlinkGCMemoryDumpProvider* BlinkGCMemoryDumpProvider::Instance() {
  DEFINE_STATIC_LOCAL(BlinkGCMemoryDumpProvider, instance, ());
  return &instance;
}

BlinkGCMemoryDumpProvider::~BlinkGCMemoryDumpProvider() = default;

bool BlinkGCMemoryDumpProvider::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* memory_dump) {
  using base::trace_event::MemoryDumpLevelOfDetail;
  MemoryDumpLevelOfDetail level_of_detail = args.level_of_detail;
  // In the case of a detailed dump perform a mark-only GC pass to collect
  // more detailed stats.
  if (level_of_detail == MemoryDumpLevelOfDetail::DETAILED) {
    ThreadState::Current()->CollectGarbage(
        BlinkGC::kNoHeapPointersOnStack, BlinkGC::kTakeSnapshot,
        BlinkGC::kEagerSweeping, BlinkGC::kForcedGC);
  }
  DumpMemoryTotals(memory_dump);

  // Merge all dumps collected by ThreadHeap::collectGarbage.
  if (level_of_detail == MemoryDumpLevelOfDetail::DETAILED)
    memory_dump->TakeAllDumpsFrom(current_process_memory_dump_.get());
  return true;
}

base::trace_event::MemoryAllocatorDump*
BlinkGCMemoryDumpProvider::CreateMemoryAllocatorDumpForCurrentGC(
    const String& absolute_name) {
  // TODO(bashi): Change type name of |absoluteName|.
  return current_process_memory_dump_->CreateAllocatorDump(
      absolute_name.Utf8().data());
}

void BlinkGCMemoryDumpProvider::ClearProcessDumpForCurrentGC() {
  current_process_memory_dump_->Clear();
}

BlinkGCMemoryDumpProvider::BlinkGCMemoryDumpProvider()
    : current_process_memory_dump_(new base::trace_event::ProcessMemoryDump(
          nullptr,
          {base::trace_event::MemoryDumpLevelOfDetail::DETAILED})) {}

}  // namespace blink
