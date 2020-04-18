// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/v8_isolate_memory_dump_provider.h"

#include <inttypes.h>
#include <stddef.h>

#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "gin/public/isolate_holder.h"
#include "v8/include/v8.h"

namespace gin {

V8IsolateMemoryDumpProvider::V8IsolateMemoryDumpProvider(
    IsolateHolder* isolate_holder,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : isolate_holder_(isolate_holder) {
  DCHECK(task_runner);
  base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
      this, "V8Isolate", task_runner);
}

V8IsolateMemoryDumpProvider::~V8IsolateMemoryDumpProvider() {
  base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
      this);
}

// Called at trace dump point time. Creates a snapshot with the memory counters
// for the current isolate.
bool V8IsolateMemoryDumpProvider::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* process_memory_dump) {
  // TODO(ssid): Use MemoryDumpArgs to create light dumps when requested
  // (crbug.com/499731).

  if (isolate_holder_->access_mode() == IsolateHolder::kUseLocker) {
    v8::Locker locked(isolate_holder_->isolate());
    DumpHeapStatistics(args, process_memory_dump);
  } else {
    DumpHeapStatistics(args, process_memory_dump);
  }
  return true;
}

namespace {

// Dump statistics related to code/bytecode when memory-infra.v8.code_stats is
// enabled.
void DumpCodeStatistics(
    base::trace_event::MemoryAllocatorDump* heap_spaces_dump,
    IsolateHolder* isolate_holder) {
  // Collecting code statistics is an expensive operation (~10 ms) when
  // compared to other v8 metrics (< 1 ms). So, dump them only when
  // memory-infra.v8.code_stats is enabled.
  // TODO(primiano): This information should be plumbed through TraceConfig.
  // See crbug.com/616441.
  bool dump_code_stats = false;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(
      TRACE_DISABLED_BY_DEFAULT("memory-infra.v8.code_stats"),
      &dump_code_stats);
  if (!dump_code_stats)
    return;

  v8::HeapCodeStatistics code_statistics;
  if (!isolate_holder->isolate()->GetHeapCodeAndMetadataStatistics(
          &code_statistics)) {
    return;
  }

  heap_spaces_dump->AddScalar(
      "code_and_metadata_size",
      base::trace_event::MemoryAllocatorDump::kUnitsBytes,
      code_statistics.code_and_metadata_size());
  heap_spaces_dump->AddScalar(
      "bytecode_and_metadata_size",
      base::trace_event::MemoryAllocatorDump::kUnitsBytes,
      code_statistics.bytecode_and_metadata_size());
  heap_spaces_dump->AddScalar(
      "external_script_source_size",
      base::trace_event::MemoryAllocatorDump::kUnitsBytes,
      code_statistics.external_script_source_size());
}

// Dump the number of native and detached contexts.
// The result looks as follows in the Chrome trace viewer:
// ======================================
// Component                 object_count
// - v8
//   - isolate
//     - contexts
//       - detached_context  10
//       - native_context    20
// ======================================
void DumpContextStatistics(
    base::trace_event::ProcessMemoryDump* process_memory_dump,
    std::string dump_base_name,
    size_t number_of_detached_contexts,
    size_t number_of_native_contexts) {
  std::string dump_name_prefix = dump_base_name + "/contexts";
  std::string native_context_name = dump_name_prefix + "/native_context";
  auto* native_context_dump =
      process_memory_dump->CreateAllocatorDump(native_context_name);
  native_context_dump->AddScalar(
      "object_count", base::trace_event::MemoryAllocatorDump::kUnitsObjects,
      number_of_native_contexts);
  std::string detached_context_name = dump_name_prefix + "/detached_context";
  auto* detached_context_dump =
      process_memory_dump->CreateAllocatorDump(detached_context_name);
  detached_context_dump->AddScalar(
      "object_count", base::trace_event::MemoryAllocatorDump::kUnitsObjects,
      number_of_detached_contexts);
}

}  // namespace anonymous

void V8IsolateMemoryDumpProvider::DumpHeapStatistics(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* process_memory_dump) {
  std::string dump_base_name = base::StringPrintf(
      "v8/isolate_0x%" PRIXPTR,
      reinterpret_cast<uintptr_t>(isolate_holder_->isolate()));

  // Dump statistics of the heap's spaces.
  std::string space_name_prefix = dump_base_name + "/heap_spaces";
  v8::HeapStatistics heap_statistics;
  isolate_holder_->isolate()->GetHeapStatistics(&heap_statistics);

  size_t known_spaces_used_size = 0;
  size_t known_spaces_size = 0;
  size_t known_spaces_physical_size = 0;
  size_t number_of_spaces = isolate_holder_->isolate()->NumberOfHeapSpaces();
  for (size_t space = 0; space < number_of_spaces; space++) {
    v8::HeapSpaceStatistics space_statistics;
    isolate_holder_->isolate()->GetHeapSpaceStatistics(&space_statistics,
                                                       space);
    const size_t space_size = space_statistics.space_size();
    const size_t space_used_size = space_statistics.space_used_size();
    const size_t space_physical_size = space_statistics.physical_space_size();

    known_spaces_size += space_size;
    known_spaces_used_size += space_used_size;
    known_spaces_physical_size += space_physical_size;

    std::string space_dump_name =
        space_name_prefix + "/" + space_statistics.space_name();
    auto* space_dump =
        process_memory_dump->CreateAllocatorDump(space_dump_name);
    space_dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                          base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                          space_physical_size);

    space_dump->AddScalar("virtual_size",
                          base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                          space_size);

    space_dump->AddScalar("allocated_objects_size",
                          base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                          space_used_size);
  }

  // Sanity checks.
  DCHECK_EQ(heap_statistics.total_physical_size(), known_spaces_physical_size);
  DCHECK_EQ(heap_statistics.used_heap_size(), known_spaces_used_size);
  DCHECK_EQ(heap_statistics.total_heap_size(), known_spaces_size);

  // If V8 zaps garbage, all the memory mapped regions become resident,
  // so we add an extra dump to avoid mismatches w.r.t. the total
  // resident values.
  if (heap_statistics.does_zap_garbage()) {
    auto* zap_dump = process_memory_dump->CreateAllocatorDump(
        dump_base_name + "/zapped_for_debug");
    zap_dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                        base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                        heap_statistics.total_heap_size() -
                            heap_statistics.total_physical_size());
  }

  // Dump statistics about malloced memory.
  std::string malloc_name = dump_base_name + "/malloc";
  auto* malloc_dump = process_memory_dump->CreateAllocatorDump(malloc_name);
  malloc_dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                         base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                         heap_statistics.malloced_memory());
  malloc_dump->AddScalar("peak_size",
                         base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                         heap_statistics.peak_malloced_memory());
  const char* system_allocator_name =
      base::trace_event::MemoryDumpManager::GetInstance()
          ->system_allocator_pool_name();
  if (system_allocator_name) {
    process_memory_dump->AddSuballocation(malloc_dump->guid(),
                                          system_allocator_name);
  }

  DumpContextStatistics(process_memory_dump, dump_base_name,
                        heap_statistics.number_of_detached_contexts(),
                        heap_statistics.number_of_native_contexts());

  // Add an empty row for the heap_spaces. This is to keep the shape of the
  // dump stable, whether code stats are enabled or not.
  auto* heap_spaces_dump =
      process_memory_dump->CreateAllocatorDump(space_name_prefix);

  // Dump statistics related to code and bytecode if requested.
  DumpCodeStatistics(heap_spaces_dump, isolate_holder_);

  // Dump object statistics only for detailed dumps.
  if (args.level_of_detail !=
      base::trace_event::MemoryDumpLevelOfDetail::DETAILED) {
    return;
  }

  // Dump statistics of the heap's live objects from last GC.
  // TODO(primiano): these should not be tracked in the same trace event as they
  // report stats for the last GC (not the current state). See crbug.com/498779.
  std::string object_name_prefix = dump_base_name + "/heap_objects_at_last_gc";
  bool did_dump_object_stats = false;
  const size_t object_types =
      isolate_holder_->isolate()->NumberOfTrackedHeapObjectTypes();
  for (size_t type_index = 0; type_index < object_types; type_index++) {
    v8::HeapObjectStatistics object_statistics;
    if (!isolate_holder_->isolate()->GetHeapObjectStatisticsAtLastGC(
            &object_statistics, type_index))
      continue;

    std::string dump_name =
        object_name_prefix + "/" + object_statistics.object_type();
    if (object_statistics.object_sub_type()[0] != '\0')
      dump_name += std::string("/") + object_statistics.object_sub_type();
    auto* object_dump = process_memory_dump->CreateAllocatorDump(dump_name);

    object_dump->AddScalar(
        base::trace_event::MemoryAllocatorDump::kNameObjectCount,
        base::trace_event::MemoryAllocatorDump::kUnitsObjects,
        object_statistics.object_count());
    object_dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                           base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                           object_statistics.object_size());
    did_dump_object_stats = true;
  }

  if (process_memory_dump->GetAllocatorDump(object_name_prefix +
                                            "/CODE_TYPE")) {
    auto* code_kind_dump = process_memory_dump->CreateAllocatorDump(
        object_name_prefix + "/CODE_TYPE/CODE_KIND");
    auto* code_age_dump = process_memory_dump->CreateAllocatorDump(
        object_name_prefix + "/CODE_TYPE/CODE_AGE");
    process_memory_dump->AddOwnershipEdge(code_kind_dump->guid(),
                                          code_age_dump->guid());
  }

  if (did_dump_object_stats) {
    process_memory_dump->AddOwnershipEdge(
        process_memory_dump->CreateAllocatorDump(object_name_prefix)->guid(),
        heap_spaces_dump->guid());
  }
}

}  // namespace gin
