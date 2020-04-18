// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation_mojom_traits.h"

namespace mojo {

// static
memory_instrumentation::mojom::DumpType
EnumTraits<memory_instrumentation::mojom::DumpType,
           base::trace_event::MemoryDumpType>::
    ToMojom(base::trace_event::MemoryDumpType type) {
  switch (type) {
    case base::trace_event::MemoryDumpType::PERIODIC_INTERVAL:
      return memory_instrumentation::mojom::DumpType::PERIODIC_INTERVAL;
    case base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED:
      return memory_instrumentation::mojom::DumpType::EXPLICITLY_TRIGGERED;
    case base::trace_event::MemoryDumpType::PEAK_MEMORY_USAGE:
      return memory_instrumentation::mojom::DumpType::PEAK_MEMORY_USAGE;
    case base::trace_event::MemoryDumpType::SUMMARY_ONLY:
      return memory_instrumentation::mojom::DumpType::SUMMARY_ONLY;
    default:
      CHECK(false) << "Invalid type: " << static_cast<uint8_t>(type);
      // This should not be reached. Just return a random value.
      return memory_instrumentation::mojom::DumpType::PEAK_MEMORY_USAGE;
  }
}

// static
bool EnumTraits<memory_instrumentation::mojom::DumpType,
                base::trace_event::MemoryDumpType>::
    FromMojom(memory_instrumentation::mojom::DumpType input,
              base::trace_event::MemoryDumpType* out) {
  switch (input) {
    case memory_instrumentation::mojom::DumpType::PERIODIC_INTERVAL:
      *out = base::trace_event::MemoryDumpType::PERIODIC_INTERVAL;
      break;
    case memory_instrumentation::mojom::DumpType::EXPLICITLY_TRIGGERED:
      *out = base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED;
      break;
    case memory_instrumentation::mojom::DumpType::PEAK_MEMORY_USAGE:
      *out = base::trace_event::MemoryDumpType::PEAK_MEMORY_USAGE;
      break;
    case memory_instrumentation::mojom::DumpType::SUMMARY_ONLY:
      *out = base::trace_event::MemoryDumpType::SUMMARY_ONLY;
      break;
    default:
      NOTREACHED() << "Invalid type: " << static_cast<uint8_t>(input);
      return false;
  }
  return true;
}

// static
memory_instrumentation::mojom::LevelOfDetail
EnumTraits<memory_instrumentation::mojom::LevelOfDetail,
           base::trace_event::MemoryDumpLevelOfDetail>::
    ToMojom(base::trace_event::MemoryDumpLevelOfDetail level_of_detail) {
  switch (level_of_detail) {
    case base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND:
      return memory_instrumentation::mojom::LevelOfDetail::BACKGROUND;
    case base::trace_event::MemoryDumpLevelOfDetail::LIGHT:
      return memory_instrumentation::mojom::LevelOfDetail::LIGHT;
    case base::trace_event::MemoryDumpLevelOfDetail::DETAILED:
      return memory_instrumentation::mojom::LevelOfDetail::DETAILED;
    default:
      CHECK(false) << "Invalid type: " << static_cast<uint8_t>(level_of_detail);
      // This should not be reached. Just return a random value.
      return memory_instrumentation::mojom::LevelOfDetail::BACKGROUND;
  }
}

// static
bool EnumTraits<memory_instrumentation::mojom::LevelOfDetail,
                base::trace_event::MemoryDumpLevelOfDetail>::
    FromMojom(memory_instrumentation::mojom::LevelOfDetail input,
              base::trace_event::MemoryDumpLevelOfDetail* out) {
  switch (input) {
    case memory_instrumentation::mojom::LevelOfDetail::BACKGROUND:
      *out = base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND;
      break;
    case memory_instrumentation::mojom::LevelOfDetail::LIGHT:
      *out = base::trace_event::MemoryDumpLevelOfDetail::LIGHT;
      break;
    case memory_instrumentation::mojom::LevelOfDetail::DETAILED:
      *out = base::trace_event::MemoryDumpLevelOfDetail::DETAILED;
      break;
    default:
      NOTREACHED() << "Invalid type: " << static_cast<uint8_t>(input);
      return false;
  }
  return true;
}

// static
memory_instrumentation::mojom::HeapProfilingMode
EnumTraits<memory_instrumentation::mojom::HeapProfilingMode,
           base::trace_event::HeapProfilingMode>::
    ToMojom(base::trace_event::HeapProfilingMode mode) {
  switch (mode) {
    case base::trace_event::HeapProfilingMode::kHeapProfilingModePseudo:
      return memory_instrumentation::mojom::HeapProfilingMode::PSEUDO_STACK;
    case base::trace_event::HeapProfilingMode::kHeapProfilingModeNative:
      return memory_instrumentation::mojom::HeapProfilingMode::NATIVE_STACK;
    case base::trace_event::HeapProfilingMode::kHeapProfilingModeBackground:
      return memory_instrumentation::mojom::HeapProfilingMode::BACKGROUND;
    case base::trace_event::HeapProfilingMode::kHeapProfilingModeTaskProfiler:
      return memory_instrumentation::mojom::HeapProfilingMode::TASK_PROFILER;
    case base::trace_event::HeapProfilingMode::kHeapProfilingModeDisabled:
      return memory_instrumentation::mojom::HeapProfilingMode::DISABLED;
    case base::trace_event::HeapProfilingMode::kHeapProfilingModeInvalid:
      break;
  }
  NOTREACHED() << "Invalid mode: " << static_cast<uint8_t>(mode);
  return memory_instrumentation::mojom::HeapProfilingMode::DISABLED;
}

// static
bool EnumTraits<memory_instrumentation::mojom::HeapProfilingMode,
                base::trace_event::HeapProfilingMode>::
    FromMojom(memory_instrumentation::mojom::HeapProfilingMode input,
              base::trace_event::HeapProfilingMode* out) {
  switch (input) {
    case memory_instrumentation::mojom::HeapProfilingMode::DISABLED:
      *out = base::trace_event::HeapProfilingMode::kHeapProfilingModeDisabled;
      return true;
    case memory_instrumentation::mojom::HeapProfilingMode::TASK_PROFILER:
      *out =
          base::trace_event::HeapProfilingMode::kHeapProfilingModeTaskProfiler;
      return true;
    case memory_instrumentation::mojom::HeapProfilingMode::BACKGROUND:
      *out = base::trace_event::HeapProfilingMode::kHeapProfilingModeBackground;
      return true;
    case memory_instrumentation::mojom::HeapProfilingMode::PSEUDO_STACK:
      *out = base::trace_event::HeapProfilingMode::kHeapProfilingModePseudo;
      return true;
    case memory_instrumentation::mojom::HeapProfilingMode::NATIVE_STACK:
      *out = base::trace_event::HeapProfilingMode::kHeapProfilingModeNative;
      return true;
  }
  NOTREACHED() << "Invalid mode: " << static_cast<uint8_t>(input);
  return false;
}

// static
bool StructTraits<memory_instrumentation::mojom::RequestArgsDataView,
                  base::trace_event::MemoryDumpRequestArgs>::
    Read(memory_instrumentation::mojom::RequestArgsDataView input,
         base::trace_event::MemoryDumpRequestArgs* out) {
  out->dump_guid = input.dump_guid();
  if (!input.ReadDumpType(&out->dump_type))
    return false;
  if (!input.ReadLevelOfDetail(&out->level_of_detail))
    return false;
  return true;
}

// static
bool StructTraits<
    memory_instrumentation::mojom::RawAllocatorDumpEdgeDataView,
    base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge>::
    Read(memory_instrumentation::mojom::RawAllocatorDumpEdgeDataView input,
         base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge* out) {
  out->source = base::trace_event::MemoryAllocatorDumpGuid(input.source_id());
  out->target = base::trace_event::MemoryAllocatorDumpGuid(input.target_id());
  out->importance = input.importance();
  out->overridable = input.overridable();
  return true;
}

// static
bool UnionTraits<
    memory_instrumentation::mojom::RawAllocatorDumpEntryValueDataView,
    base::trace_event::MemoryAllocatorDump::Entry>::
    Read(
        memory_instrumentation::mojom::RawAllocatorDumpEntryValueDataView input,
        base::trace_event::MemoryAllocatorDump::Entry* out) {
  using memory_instrumentation::mojom::RawAllocatorDumpEntryValue;
  switch (input.tag()) {
    case RawAllocatorDumpEntryValue::Tag::VALUE_STRING: {
      std::string value_string;
      if (!input.ReadValueString(&value_string))
        return false;
      out->value_string = std::move(value_string);
      out->entry_type = base::trace_event::MemoryAllocatorDump::Entry::kString;
      break;
    }
    case RawAllocatorDumpEntryValue::Tag::VALUE_UINT64: {
      out->value_uint64 = input.value_uint64();
      out->entry_type = base::trace_event::MemoryAllocatorDump::Entry::kUint64;
      break;
    }
    default:
      return false;
  }
  return true;
}

// static
bool StructTraits<memory_instrumentation::mojom::RawAllocatorDumpEntryDataView,
                  base::trace_event::MemoryAllocatorDump::Entry>::
    Read(memory_instrumentation::mojom::RawAllocatorDumpEntryDataView input,
         base::trace_event::MemoryAllocatorDump::Entry* out) {
  if (!input.ReadName(&out->name) || !input.ReadUnits(&out->units))
    return false;
  if (!input.ReadValue(out))
    return false;
  return true;
}

// static
bool StructTraits<memory_instrumentation::mojom::RawAllocatorDumpDataView,
                  std::unique_ptr<base::trace_event::MemoryAllocatorDump>>::
    Read(memory_instrumentation::mojom::RawAllocatorDumpDataView input,
         std::unique_ptr<base::trace_event::MemoryAllocatorDump>* out) {
  std::string absolute_name;
  if (!input.ReadAbsoluteName(&absolute_name))
    return false;
  base::trace_event::MemoryDumpLevelOfDetail level_of_detail;
  if (!input.ReadLevelOfDetail(&level_of_detail))
    return false;
  auto mad = std::make_unique<base::trace_event::MemoryAllocatorDump>(
      absolute_name, level_of_detail,
      base::trace_event::MemoryAllocatorDumpGuid(input.id()));
  if (input.weak())
    mad->set_flags(base::trace_event::MemoryAllocatorDump::WEAK);
  if (!input.ReadEntries(mad->mutable_entries_for_serialization()))
    return false;
  *out = std::move(mad);
  return true;
}

// static
bool StructTraits<memory_instrumentation::mojom::RawProcessMemoryDumpDataView,
                  std::unique_ptr<base::trace_event::ProcessMemoryDump>>::
    Read(memory_instrumentation::mojom::RawProcessMemoryDumpDataView input,
         std::unique_ptr<base::trace_event::ProcessMemoryDump>* out) {
  base::trace_event::MemoryDumpArgs dump_args;
  if (!input.ReadLevelOfDetail(&dump_args.level_of_detail))
    return false;
  std::vector<base::trace_event::ProcessMemoryDump::MemoryAllocatorDumpEdge>
      edges;
  if (!input.ReadAllocatorDumpEdges(&edges))
    return false;
  std::vector<std::unique_ptr<base::trace_event::MemoryAllocatorDump>> dumps;
  if (!input.ReadAllocatorDumps(&dumps))
    return false;
  auto pmd = std::make_unique<base::trace_event::ProcessMemoryDump>(nullptr,
                                                                    dump_args);
  pmd->SetAllocatorDumpsForSerialization(std::move(dumps));
  pmd->SetAllEdgesForSerialization(edges);
  *out = std::move(pmd);
  return true;
}

}  // namespace mojo
