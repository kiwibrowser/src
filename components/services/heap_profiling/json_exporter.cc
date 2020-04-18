// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/json_exporter.h"

#include <map>

#include "base/containers/adapters.h"
#include "base/format_macros.h"
#include "base/json/json_writer.h"
#include "base/json/string_escape.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/tracing_observer.h"

namespace heap_profiling {

namespace {

// Maps strings to integers for the JSON string table.
using StringTable = std::map<std::string, size_t>;

constexpr uint32_t kAllocatorCount =
    static_cast<uint32_t>(AllocatorType::kCount);

struct BacktraceNode {
  BacktraceNode(size_t string_id, size_t parent)
      : string_id_(string_id), parent_(parent) {}

  static constexpr size_t kNoParent = static_cast<size_t>(-1);

  size_t string_id() const { return string_id_; }
  size_t parent() const { return parent_; }

  bool operator<(const BacktraceNode& other) const {
    return std::tie(string_id_, parent_) <
           std::tie(other.string_id_, other.parent_);
  }

 private:
  const size_t string_id_;
  const size_t parent_;  // kNoParent indicates no parent.
};

using BacktraceTable = std::map<BacktraceNode, size_t>;

// Used as a temporary map key to uniquify an allocation with a given context
// and stack. A lock is held when dumping bactraces that guarantees that no
// Backtraces will be created or destroyed during the lifetime of that
// structure. Therefore it's safe to use a raw pointer Since backtraces are
// uniquified, this does pointer comparisons on the backtrace to give a stable
// ordering, even if that ordering has no intrinsic meaning.
struct UniqueAlloc {
 public:
  UniqueAlloc(const Backtrace* bt, int ctx_id)
      : backtrace(bt), context_id(ctx_id) {}

  bool operator<(const UniqueAlloc& other) const {
    return std::tie(backtrace, context_id) <
           std::tie(other.backtrace, other.context_id);
  }

  const Backtrace* backtrace = nullptr;
  const int context_id = 0;
};

struct UniqueAllocMetrics {
  size_t size = 0;
  size_t count = 0;
};

using UniqueAllocationMap = std::map<UniqueAlloc, UniqueAllocMetrics>;

// The hardcoded ID for having no context for an allocation.
constexpr int kUnknownTypeId = 0;

const char* StringForAllocatorType(uint32_t type) {
  switch (static_cast<AllocatorType>(type)) {
    case AllocatorType::kMalloc:
      return "malloc";
    case AllocatorType::kPartitionAlloc:
      return "partition_alloc";
    case AllocatorType::kOilpan:
      return "blink_gc";
    default:
      NOTREACHED();
      return "unknown";
  }
}

// Writes the top-level allocators section. This section is used by the tracing
// UI to show a small summary for each allocator. It's necessary as a
// placeholder to allow the stack-viewing UI to be shown.
//
// Each array should be the number of allocators long.
void WriteAllocatorsSummary(size_t total_size[],
                            size_t total_count[],
                            std::ostream& out) {
  out << "\"allocators\":{\n";
  for (uint32_t i = 0; i < kAllocatorCount; i++) {
    const char* alloc_type = StringForAllocatorType(i);

    // Overall sizes.
    static constexpr char kAttrsSizeBody[] = R"(
      "%s": {
        "attrs": {
          "virtual_size": {
            "type": "scalar",
            "units": "bytes",
            "value": "%zx"
          },
          "size": {
            "type": "scalar",
            "units": "bytes",
            "value": "%zx"
          }
        }
      },)";
    out << base::StringPrintf(kAttrsSizeBody, alloc_type, total_size[i],
                              total_size[i]);

    // Allocated objects.
    static constexpr char kAttrsObjectsBody[] = R"(
      "%s/allocated_objects": {
        "attrs": {
          "shim_allocated_objects_count": {
            "type": "scalar",
            "units": "objects",
            "value": "%zx"
          },
          "shim_allocated_objects_size": {
            "type": "scalar",
            "units": "bytes",
            "value": "%zx"
          }
        }
      })";
    out << base::StringPrintf(kAttrsObjectsBody, alloc_type, total_count[i],
                              total_size[i]);

    // Comma except for the last time.
    if (i < kAllocatorCount - 1)
      out << ',';
    out << "\n";
  }
  out << "},\n";
}

// Writes the dictionary keys to preceed a "heaps_v2" trace argument inside a
// "dumps". This is "v2" heap dump format.
void WriteHeapsV2Header(std::ostream& out) {
  out << "\"heaps_v2\": {\n";
}

// Closes the dictionaries from the WriteHeapsV2Header function above.
void WriteHeapsV2Footer(std::ostream& out) {
  out << "}";  // heaps_v2
}

void WriteMemoryMaps(const ExportParams& params, std::ostream& out) {
  base::trace_event::TracedValue traced_value;
  memory_instrumentation::TracingObserver::MemoryMapsAsValueInto(
      params.maps, &traced_value, params.strip_path_from_mapped_files);
  out << "\"process_mmaps\":" << traced_value.ToString();
}

// Inserts or retrieves the ID for a string in the string table.
size_t AddOrGetString(const std::string& str,
                      StringTable* string_table,
                      ExportParams* params) {
  auto result = string_table->emplace(str, params->next_id++);
  // "result.first" is an iterator into the map.
  return result.first->second;
}

// Processes the context information needed for the give set of allocations.
// Strings are added for each referenced context and a mapping between
// context IDs and string IDs is filled in for each.
void FillContextStrings(const UniqueAllocationMap& allocations,
                        ExportParams* params,
                        StringTable* string_table,
                        std::map<int, size_t>* context_to_string_map) {
  std::set<int> used_context;
  for (const auto& alloc : allocations)
    used_context.insert(alloc.first.context_id);

  if (used_context.find(kUnknownTypeId) != used_context.end()) {
    // Hard code a string for the unknown context type.
    context_to_string_map->emplace(
        kUnknownTypeId, AddOrGetString("[unknown]", string_table, params));
  }

  // The context map is backwards from what we need, so iterate through the
  // whole thing and see which ones are used.
  for (const auto& context : params->context_map) {
    if (used_context.find(context.second) != used_context.end()) {
      size_t string_id = AddOrGetString(context.first, string_table, params);
      context_to_string_map->emplace(context.second, string_id);
    }
  }
}

size_t AddOrGetBacktraceNode(BacktraceNode node,
                             BacktraceTable* backtrace_table,
                             ExportParams* params) {
  auto result = backtrace_table->emplace(std::move(node), params->next_id++);
  // "result.first" is an iterator into the map.
  return result.first->second;
}

// Returns the index into nodes of the node to reference for this stack. That
// node will reference its parent node, etc. to allow the full stack to
// be represented.
size_t AppendBacktraceStrings(const Backtrace& backtrace,
                              BacktraceTable* backtrace_table,
                              StringTable* string_table,
                              ExportParams* params) {
  int parent = -1;
  // Addresses must be outputted in reverse order.
  for (const Address& addr : base::Reversed(backtrace.addrs())) {
    size_t sid;
    auto it = params->mapped_strings.find(addr.value);
    if (it != params->mapped_strings.end()) {
      sid = AddOrGetString(it->second, string_table, params);
    } else {
      static constexpr char kPcPrefix[] = "pc:";
      // std::numeric_limits<>::digits gives the number of bits in the value.
      // Dividing by 4 gives the number of hex digits needed to store the value.
      // Adding to sizeof(kPcPrefix) yields the buffer size needed including the
      // null terminator.
      static constexpr int kBufSize =
          sizeof(kPcPrefix) +
          (std::numeric_limits<decltype(addr.value)>::digits / 4);
      char buf[kBufSize];
      snprintf(buf, kBufSize, "%s%" PRIx64, kPcPrefix, addr.value);
      sid = AddOrGetString(buf, string_table, params);
    }
    parent = AddOrGetBacktraceNode(BacktraceNode(sid, parent), backtrace_table,
                                   params);
  }
  return parent;  // Last item is the end of this stack.
}

// Writes the string table which looks like:
//   "strings":[
//     {"id":123,string:"This is the string"},
//     ...
//   ]
void WriteStrings(const StringTable& string_table, std::ostream& out) {
  out << "\"strings\":[";
  bool first_time = true;
  for (const auto& string_pair : string_table) {
    if (!first_time)
      out << ",\n";
    else
      first_time = false;

    out << "{\"id\":" << string_pair.second;
    // TODO(brettw) when we have real symbols this will need escaping.
    out << ",\"string\":\"" << string_pair.first << "\"}";
  }
  out << "]";
}

// Writes the nodes array in the maps section. These are all the backtrace
// entries and are indexed by the allocator nodes array.
//   "nodes":[
//     {"id":1, "name_sid":123, "parent":17},
//     ...
//   ]
void WriteMapNodes(const BacktraceTable& nodes, std::ostream& out) {
  out << "\"nodes\":[";

  bool first_time = true;
  for (const auto& node : nodes) {
    if (!first_time)
      out << ",\n";
    else
      first_time = false;

    out << "{\"id\":" << node.second;
    out << ",\"name_sid\":" << node.first.string_id();
    if (node.first.parent() != BacktraceNode::kNoParent)
      out << ",\"parent\":" << node.first.parent();
    out << "}";
  }
  out << "]";
}

// Write the types array in the maps section. These are the context for each
// allocation and just maps IDs to string IDs.
//    "types":[
//      {"id":1, "name_sid":123},
//    ]
void WriteTypeNodes(const std::map<int, size_t>& type_to_string,
                    std::ostream& out) {
  out << "\"types\":[";

  bool first_time = true;
  for (const auto& type : type_to_string) {
    if (!first_time)
      out << ",\n";
    else
      first_time = false;

    out << "{\"id\":" << type.first << ",\"name_sid\":" << type.second << "}";
  }
  out << "]";
}

// Writes the number of matching allocations array which looks like:
//   "counts":[1, 1, 2]
void WriteCounts(const UniqueAllocationMap& allocations, std::ostream& out) {
  out << "\"counts\":[";
  bool first_time = true;
  for (const auto& cur : allocations) {
    if (!first_time)
      out << ",";
    else
      first_time = false;
    out << cur.second.count;
  }
  out << "]";
}

// Writes the total sizes of each allocation which looks like:
//   "sizes":[32, 64, 12]
void WriteSizes(const UniqueAllocationMap& allocations, std::ostream& out) {
  out << "\"sizes\":[";
  bool first_time = true;
  for (const auto& cur : allocations) {
    if (!first_time)
      out << ",";
    else
      first_time = false;
    out << cur.second.size;
  }
  out << "]";
}

// Writes the types array of integers which looks like:
//   "types":[0, 0, 1]
void WriteTypes(const UniqueAllocationMap& allocations, std::ostream& out) {
  out << "\"types\":[";
  bool first_time = true;
  for (const auto& cur : allocations) {
    if (!first_time)
      out << ",";
    else
      first_time = false;
    out << cur.first.context_id;
  }
  out << "]";
}

// Writes the nodes array which indexes for each allocation into the maps nodes
// array written above. It looks like:
//   "nodes":[1, 5, 10]
void WriteAllocatorNodes(const UniqueAllocationMap& allocations,
                         const std::map<const Backtrace*, size_t>& backtraces,
                         std::ostream& out) {
  out << "\"nodes\":[";
  bool first_time = true;
  for (const auto& cur : allocations) {
    if (!first_time)
      out << ",";
    else
      first_time = false;
    auto found = backtraces.find(cur.first.backtrace);
    out << found->second;
  }
  out << "]";
}

}  // namespace

ExportParams::ExportParams() = default;
ExportParams::~ExportParams() = default;

void ExportMemoryMapsAndV2StackTraceToJSON(ExportParams* params,
                                           std::ostream& out) {
  // Start dictionary.
  out << "{\n";

  WriteMemoryMaps(*params, out);
  out << ",\n";

  // Write level of detail.
  out << R"("level_of_detail": "detailed")"
      << ",\n";

  // Aggregate stats for each allocator type.
  size_t total_size[kAllocatorCount] = {0};
  size_t total_count[kAllocatorCount] = {0};
  UniqueAllocationMap filtered_allocations[kAllocatorCount];
  for (const auto& alloc_pair : params->allocs) {
    uint32_t allocator_index =
        static_cast<uint32_t>(alloc_pair.first.allocator());
    size_t alloc_count = alloc_pair.second;
    size_t alloc_size = alloc_pair.first.size();
    size_t alloc_total_size = alloc_size * alloc_count;

    // If allocations were sampled, then we need to desample to return accurate
    // results.
    if (alloc_size < params->sampling_rate && alloc_size != 0) {
      // To desample, we need to know the probability P that an allocation will
      // be sampled. Once we know P, we still have to deal with discretization.
      // Let's say that there's 1 allocation with P=0.85. Should we report 1 or
      // 2 allocations? Should we report a fudged size (size / 0.85), or a
      // discreted size, e.g. (1 * size) or (2 * size)? There are tradeoffs.
      //
      // We choose to emit a fudged size, which will return a more accurate
      // total allocation size, but less accurate per-allocation size.
      //
      // The aggregate probability that an allocation will be sampled is
      // alloc_size / sampling_rate. For a more detailed treatise, see
      // https://bugs.chromium.org/p/chromium/issues/detail?id=810748#c4
      float desampling_multiplier = static_cast<float>(params->sampling_rate) /
                                    static_cast<float>(alloc_size);
      alloc_count *= desampling_multiplier;
      alloc_total_size *= desampling_multiplier;
    }

    total_size[allocator_index] += alloc_total_size;
    total_count[allocator_index] += alloc_count;

    UniqueAlloc unique_alloc(alloc_pair.first.backtrace(),
                             alloc_pair.first.context_id());
    UniqueAllocMetrics& unique_alloc_metrics =
        filtered_allocations[allocator_index][unique_alloc];
    unique_alloc_metrics.size += alloc_total_size;
    unique_alloc_metrics.count += alloc_count;
  }

  // Filter irrelevant allocations.
  // TODO(crbug:763595): Leave placeholders for pruned allocations.
  for (uint32_t i = 0; i < kAllocatorCount; i++) {
    for (auto alloc = filtered_allocations[i].begin();
         alloc != filtered_allocations[i].end();) {
      if (alloc->second.size < params->min_size_threshold &&
          alloc->second.count < params->min_count_threshold) {
        alloc = filtered_allocations[i].erase(alloc);
      } else {
        ++alloc;
      }
    }
  }

  WriteAllocatorsSummary(total_size, total_count, out);
  WriteHeapsV2Header(out);

  // Output Heaps_V2 format version. Currently "1" is the only valid value.
  out << "\"version\": 1,\n";

  StringTable string_table;

  // Put all required context strings in the string table and generate a
  // mapping from allocation context_id to string ID.
  std::map<int, size_t> context_to_string_map;
  for (uint32_t i = 0; i < kAllocatorCount; i++) {
    FillContextStrings(filtered_allocations[i], params, &string_table,
                       &context_to_string_map);
  }

  // Find all backtraces referenced by the set and not filtered. The backtrace
  // storage will contain more stacks than we want to write out (it will refer
  // to all processes, while we're only writing one). So do those only on
  // demand.
  //
  // The map maps backtrace keys to node IDs (computed below).
  std::map<const Backtrace*, size_t> backtraces;
  for (size_t i = 0; i < kAllocatorCount; i++) {
    for (const auto& alloc : filtered_allocations[i])
      backtraces.emplace(alloc.first.backtrace, 0);
  }

  // Write each backtrace, converting the string for the stack entry to string
  // IDs. The backtrace -> node ID will be filled in at this time.
  BacktraceTable nodes;
  VLOG(1) << "Number of backtraces " << backtraces.size();
  for (auto& bt : backtraces)
    bt.second =
        AppendBacktraceStrings(*bt.first, &nodes, &string_table, params);

  // Maps section.
  out << "\"maps\": {\n";
  WriteStrings(string_table, out);
  out << ",\n";
  WriteMapNodes(nodes, out);
  out << ",\n";
  WriteTypeNodes(context_to_string_map, out);
  out << "},\n";  // End of maps section.

  // Allocators section.
  out << "\"allocators\":{\n";
  for (uint32_t i = 0; i < kAllocatorCount; i++) {
    out << "  \"" << StringForAllocatorType(i) << "\":{\n    ";

    WriteCounts(filtered_allocations[i], out);
    out << ",\n    ";
    WriteSizes(filtered_allocations[i], out);
    out << ",\n    ";
    WriteTypes(filtered_allocations[i], out);
    out << ",\n    ";
    WriteAllocatorNodes(filtered_allocations[i], backtraces, out);
    out << "\n  }";

    // Comma every time but the last.
    if (i < kAllocatorCount - 1)
      out << ',';
    out << "\n";
  }
  out << "}\n";  // End of allocators section.

  WriteHeapsV2Footer(out);

  // End dictionary.
  out << "}\n";
}

}  // namespace heap_profiling
