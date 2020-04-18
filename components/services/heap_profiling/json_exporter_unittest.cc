// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/json_exporter.h"

#include <sstream>

#include "base/gtest_prod_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/services/heap_profiling/backtrace_storage.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/os_metrics.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace heap_profiling {

namespace {

const size_t kNoSizeThreshold = 0;
const size_t kNoCountThreshold = 0;
const size_t kSizeThreshold = 1500;
const size_t kCountThreshold = 1000;

using MemoryMap = std::vector<memory_instrumentation::mojom::VmRegionPtr>;

static constexpr int kNoParent = -1;

// Finds the first vm region in the given periodic interval. Returns null on
// failure.
const base::Value* FindFirstRegionWithAnyName(const base::Value* root) {
  const base::Value* found_mmaps =
      root->FindKeyOfType("process_mmaps", base::Value::Type::DICTIONARY);
  if (!found_mmaps)
    return nullptr;
  const base::Value* found_regions =
      found_mmaps->FindKeyOfType("vm_regions", base::Value::Type::LIST);
  if (!found_regions)
    return nullptr;

  for (const base::Value& cur : found_regions->GetList()) {
    const base::Value* found_name =
        cur.FindKeyOfType("mf", base::Value::Type::STRING);
    if (!found_name)
      return nullptr;
    if (found_name->GetString() != "")
      return &cur;
  }
  return nullptr;
}

// Looks up a given string id from the string table. Returns -1 if not found.
int GetIdFromStringTable(const base::Value* strings, const char* text) {
  for (const auto& string : strings->GetList()) {
    const base::Value* string_id =
        string.FindKeyOfType("id", base::Value::Type::INTEGER);
    const base::Value* string_text =
        string.FindKeyOfType("string", base::Value::Type::STRING);
    if (string_id != nullptr && string_text != nullptr &&
        string_text->GetString() == text)
      return string_id->GetInt();
  }
  return -1;
}

// Looks up a given string from the string table. Returns empty string if not
// found.
std::string GetStringFromStringTable(const base::Value* strings, int sid) {
  for (const auto& string : strings->GetList()) {
    const base::Value* string_id =
        string.FindKeyOfType("id", base::Value::Type::INTEGER);
    if (string_id->GetInt() == sid) {
      const base::Value* string_text =
          string.FindKeyOfType("string", base::Value::Type::STRING);
      if (!string_text)
        return std::string();
      return string_text->GetString();
    }
  }
  return std::string();
}

int GetNodeWithNameID(const base::Value* nodes, int sid) {
  for (const auto& node : nodes->GetList()) {
    const base::Value* node_id =
        node.FindKeyOfType("id", base::Value::Type::INTEGER);
    const base::Value* node_name_sid =
        node.FindKeyOfType("name_sid", base::Value::Type::INTEGER);
    if (node_id != nullptr && node_name_sid != nullptr &&
        node_name_sid->GetInt() == sid)
      return node_id->GetInt();
  }
  return -1;
}

int GetOffsetForBacktraceID(const base::Value* nodes, int id) {
  int offset = 0;
  for (const auto& node : nodes->GetList()) {
    if (node.GetInt() == id)
      return offset;
    offset++;
  }
  return -1;
}

bool IsBacktraceInList(const base::Value* backtraces, int id, int parent) {
  for (const auto& backtrace : backtraces->GetList()) {
    const base::Value* backtrace_id =
        backtrace.FindKeyOfType("id", base::Value::Type::INTEGER);
    if (backtrace_id == nullptr)
      continue;

    const base::Value* backtrace_parent =
        backtrace.FindKeyOfType("parent", base::Value::Type::INTEGER);
    int backtrace_parent_int = kNoParent;
    if (backtrace_parent)
      backtrace_parent_int = backtrace_parent->GetInt();

    if (backtrace_id->GetInt() == id && backtrace_parent_int == parent)
      return true;
  }
  return false;
}

}  // namespace

TEST(ProfilingJsonExporterTest, Simple) {
  BacktraceStorage backtrace_storage;

  std::vector<Address> stack1;
  stack1.push_back(Address(0x5678));
  stack1.push_back(Address(0x1234));
  const Backtrace* bt1 = backtrace_storage.Insert(std::move(stack1));

  std::vector<Address> stack2;
  stack2.push_back(Address(0x9013));
  stack2.push_back(Address(0x9012));
  stack2.push_back(Address(0x1234));
  const Backtrace* bt2 = backtrace_storage.Insert(std::move(stack2));

  AllocationEventSet events;
  events.insert(
      AllocationEvent(AllocatorType::kMalloc, Address(0x1), 20, bt1, 0));
  events.insert(
      AllocationEvent(AllocatorType::kMalloc, Address(0x2), 32, bt2, 0));
  events.insert(
      AllocationEvent(AllocatorType::kMalloc, Address(0x3), 20, bt1, 0));
  events.insert(AllocationEvent(AllocatorType::kPartitionAlloc, Address(0x4),
                                20, bt1, 0));
  events.insert(
      AllocationEvent(AllocatorType::kMalloc, Address(0x5), 12, bt2, 0));

  std::ostringstream stream;

  ExportParams params;
  params.allocs = AllocationEventSetToCountMap(events);
  params.min_size_threshold = kNoSizeThreshold;
  params.min_count_threshold = kNoCountThreshold;
  ExportMemoryMapsAndV2StackTraceToJSON(&params, stream);
  std::string json = stream.str();

  // JSON should parse.
  base::JSONReader reader(base::JSON_PARSE_RFC);
  std::unique_ptr<base::Value> root = reader.ReadToValue(stream.str());
  ASSERT_EQ(base::JSONReader::JSON_NO_ERROR, reader.error_code())
      << reader.GetErrorMessage();
  ASSERT_TRUE(root);

  // Validate the allocators summary.
  const base::Value* malloc_summary = root->FindPath({"allocators", "malloc"});
  ASSERT_TRUE(malloc_summary);
  const base::Value* malloc_size =
      malloc_summary->FindPath({"attrs", "size", "value"});
  ASSERT_TRUE(malloc_size);
  EXPECT_EQ("54", malloc_size->GetString());
  const base::Value* malloc_virtual_size =
      malloc_summary->FindPath({"attrs", "virtual_size", "value"});
  ASSERT_TRUE(malloc_virtual_size);
  EXPECT_EQ("54", malloc_virtual_size->GetString());

  const base::Value* partition_alloc_summary =
      root->FindPath({"allocators", "partition_alloc"});
  ASSERT_TRUE(partition_alloc_summary);
  const base::Value* partition_alloc_size =
      partition_alloc_summary->FindPath({"attrs", "size", "value"});
  ASSERT_TRUE(partition_alloc_size);
  EXPECT_EQ("14", partition_alloc_size->GetString());
  const base::Value* partition_alloc_virtual_size =
      partition_alloc_summary->FindPath({"attrs", "virtual_size", "value"});
  ASSERT_TRUE(partition_alloc_virtual_size);
  EXPECT_EQ("14", partition_alloc_virtual_size->GetString());

  const base::Value* heaps_v2 = root->FindKey("heaps_v2");
  ASSERT_TRUE(heaps_v2);

  // Retrieve maps and validate their structure.
  const base::Value* nodes = heaps_v2->FindPath({"maps", "nodes"});
  const base::Value* strings = heaps_v2->FindPath({"maps", "strings"});
  ASSERT_TRUE(nodes);
  ASSERT_TRUE(strings);

  // Validate the strings table.
  EXPECT_EQ(5u, strings->GetList().size());
  int sid_unknown = GetIdFromStringTable(strings, "[unknown]");
  int sid_1234 = GetIdFromStringTable(strings, "pc:1234");
  int sid_5678 = GetIdFromStringTable(strings, "pc:5678");
  int sid_9012 = GetIdFromStringTable(strings, "pc:9012");
  int sid_9013 = GetIdFromStringTable(strings, "pc:9013");
  EXPECT_NE(-1, sid_unknown);
  EXPECT_NE(-1, sid_1234);
  EXPECT_NE(-1, sid_5678);
  EXPECT_NE(-1, sid_9012);
  EXPECT_NE(-1, sid_9013);

  // Validate the nodes table.
  // Nodes should be a list with 4 items.
  //   [0] => address: 1234  parent: none
  //   [1] => address: 5678  parent: 0
  //   [2] => address: 9012  parent: 0
  //   [3] => address: 9013  parent: 2
  EXPECT_EQ(4u, nodes->GetList().size());
  int id0 = GetNodeWithNameID(nodes, sid_1234);
  int id1 = GetNodeWithNameID(nodes, sid_5678);
  int id2 = GetNodeWithNameID(nodes, sid_9012);
  int id3 = GetNodeWithNameID(nodes, sid_9013);
  EXPECT_NE(-1, id0);
  EXPECT_NE(-1, id1);
  EXPECT_NE(-1, id2);
  EXPECT_NE(-1, id3);
  EXPECT_TRUE(IsBacktraceInList(nodes, id0, kNoParent));
  EXPECT_TRUE(IsBacktraceInList(nodes, id1, id0));
  EXPECT_TRUE(IsBacktraceInList(nodes, id2, id0));
  EXPECT_TRUE(IsBacktraceInList(nodes, id3, id2));

  // Retrieve the allocations and validate their structure.
  const base::Value* counts =
      heaps_v2->FindPath({"allocators", "malloc", "counts"});
  const base::Value* types =
      heaps_v2->FindPath({"allocators", "malloc", "types"});
  const base::Value* sizes =
      heaps_v2->FindPath({"allocators", "malloc", "sizes"});
  const base::Value* backtraces =
      heaps_v2->FindPath({"allocators", "malloc", "nodes"});

  ASSERT_TRUE(counts);
  ASSERT_TRUE(types);
  ASSERT_TRUE(sizes);
  ASSERT_TRUE(backtraces);

  // Counts should be a list of two items, a 1 and a 2. The two matching 20-byte
  // allocations should be coalesced to produce the 2.
  EXPECT_EQ(2u, counts->GetList().size());
  EXPECT_EQ(2u, types->GetList().size());
  EXPECT_EQ(2u, sizes->GetList().size());

  int node1 = GetOffsetForBacktraceID(backtraces, id1);
  int node3 = GetOffsetForBacktraceID(backtraces, id3);
  EXPECT_NE(-1, node1);
  EXPECT_NE(-1, node3);

  // Validate node allocated with |stack1|.
  EXPECT_EQ(2, counts->GetList()[node1].GetInt());
  EXPECT_EQ(0, types->GetList()[node1].GetInt());
  EXPECT_EQ(40, sizes->GetList()[node1].GetInt());
  EXPECT_EQ(id1, backtraces->GetList()[node1].GetInt());

  // Validate node allocated with |stack2|.
  EXPECT_EQ(2, counts->GetList()[node3].GetInt());
  EXPECT_EQ(0, types->GetList()[node3].GetInt());
  EXPECT_EQ(44, sizes->GetList()[node3].GetInt());
  EXPECT_EQ(id3, backtraces->GetList()[node3].GetInt());

  // Validate that the partition alloc one got through.
  counts = heaps_v2->FindPath({"allocators", "partition_alloc", "counts"});
  types = heaps_v2->FindPath({"allocators", "partition_alloc", "types"});
  sizes = heaps_v2->FindPath({"allocators", "partition_alloc", "sizes"});
  backtraces = heaps_v2->FindPath({"allocators", "partition_alloc", "nodes"});

  ASSERT_TRUE(counts);
  ASSERT_TRUE(types);
  ASSERT_TRUE(sizes);
  ASSERT_TRUE(backtraces);

  // There should just be one entry for the partition_alloc allocation.
  EXPECT_EQ(1u, counts->GetList().size());
  EXPECT_EQ(1u, types->GetList().size());
  EXPECT_EQ(1u, sizes->GetList().size());
}

TEST(ProfilingJsonExporterTest, Sampling) {
  size_t allocation_size = 20;
  int sampling_rate = 1000;
  int expected_count = static_cast<int>(sampling_rate / allocation_size);

  BacktraceStorage backtrace_storage;

  std::vector<Address> stack1;
  stack1.push_back(Address(0x5678));
  const Backtrace* bt1 = backtrace_storage.Insert(std::move(stack1));

  AllocationEventSet events;
  events.insert(AllocationEvent(AllocatorType::kMalloc, Address(0x1),
                                allocation_size, bt1, 0));

  std::ostringstream stream;

  ExportParams params;
  params.allocs = AllocationEventSetToCountMap(events);
  params.min_size_threshold = kNoSizeThreshold;
  params.min_count_threshold = kNoCountThreshold;
  params.sampling_rate = sampling_rate;
  ExportMemoryMapsAndV2StackTraceToJSON(&params, stream);
  std::string json = stream.str();

  // JSON should parse.
  base::JSONReader reader(base::JSON_PARSE_RFC);
  std::unique_ptr<base::Value> root = reader.ReadToValue(stream.str());
  ASSERT_EQ(base::JSONReader::JSON_NO_ERROR, reader.error_code())
      << reader.GetErrorMessage();
  ASSERT_TRUE(root);

  // Validate the allocators summary.
  const base::Value* malloc_summary = root->FindPath({"allocators", "malloc"});
  ASSERT_TRUE(malloc_summary);
  const base::Value* malloc_size =
      malloc_summary->FindPath({"attrs", "size", "value"});
  ASSERT_TRUE(malloc_size);
  EXPECT_EQ("3e8", malloc_size->GetString());

  const base::Value* heaps_v2 = root->FindKey("heaps_v2");
  ASSERT_TRUE(heaps_v2);

  // Retrieve the allocations and validate their structure.
  const base::Value* sizes =
      heaps_v2->FindPath({"allocators", "malloc", "sizes"});

  ASSERT_TRUE(sizes);
  EXPECT_EQ(1u, sizes->GetList().size());
  EXPECT_EQ(sampling_rate, sizes->GetList()[0].GetInt());

  const base::Value* counts =
      heaps_v2->FindPath({"allocators", "malloc", "counts"});
  ASSERT_TRUE(counts);
  EXPECT_EQ(1u, counts->GetList().size());
  EXPECT_EQ(expected_count, counts->GetList()[0].GetInt());
}

TEST(ProfilingJsonExporterTest, SimpleWithFilteredAllocations) {
  BacktraceStorage backtrace_storage;

  std::vector<Address> stack1;
  stack1.push_back(Address(0x1234));
  const Backtrace* bt1 = backtrace_storage.Insert(std::move(stack1));

  std::vector<Address> stack2;
  stack2.push_back(Address(0x5678));
  const Backtrace* bt2 = backtrace_storage.Insert(std::move(stack2));

  std::vector<Address> stack3;
  stack3.push_back(Address(0x9999));
  const Backtrace* bt3 = backtrace_storage.Insert(std::move(stack3));

  AllocationEventSet events;
  events.insert(
      AllocationEvent(AllocatorType::kMalloc, Address(0x1), 16, bt1, 0));
  events.insert(
      AllocationEvent(AllocatorType::kMalloc, Address(0x2), 32, bt1, 0));
  events.insert(
      AllocationEvent(AllocatorType::kMalloc, Address(0x3), 1000, bt2, 0));
  events.insert(
      AllocationEvent(AllocatorType::kMalloc, Address(0x4), 1000, bt2, 0));
  for (size_t i = 0; i < kCountThreshold + 1; ++i) {
    events.insert(
        AllocationEvent(AllocatorType::kMalloc, Address(0x5 + i), 1, bt3, 0));
  }

  // Validate filtering by size and count.
  std::ostringstream stream;

  ExportParams params;
  params.allocs = AllocationEventSetToCountMap(events);
  params.min_size_threshold = kSizeThreshold;
  params.min_count_threshold = kCountThreshold;
  ExportMemoryMapsAndV2StackTraceToJSON(&params, stream);
  std::string json = stream.str();

  // JSON should parse.
  base::JSONReader reader(base::JSON_PARSE_RFC);
  std::unique_ptr<base::Value> root = reader.ReadToValue(stream.str());
  ASSERT_EQ(base::JSONReader::JSON_NO_ERROR, reader.error_code())
      << reader.GetErrorMessage();
  ASSERT_TRUE(root);

  const base::Value* heaps_v2 = root->FindKey("heaps_v2");
  ASSERT_TRUE(heaps_v2);
  const base::Value* nodes = heaps_v2->FindPath({"maps", "nodes"});
  const base::Value* strings = heaps_v2->FindPath({"maps", "strings"});
  ASSERT_TRUE(nodes);
  ASSERT_TRUE(strings);

  // Validate the strings table.
  EXPECT_EQ(3u, strings->GetList().size());
  int sid_unknown = GetIdFromStringTable(strings, "[unknown]");
  int sid_1234 = GetIdFromStringTable(strings, "pc:1234");
  int sid_5678 = GetIdFromStringTable(strings, "pc:5678");
  int sid_9999 = GetIdFromStringTable(strings, "pc:9999");
  EXPECT_NE(-1, sid_unknown);
  EXPECT_EQ(-1, sid_1234);  // Must be filtered.
  EXPECT_NE(-1, sid_5678);
  EXPECT_NE(-1, sid_9999);

  // Validate the nodes table.
  // Nodes should be a list with 4 items.
  //   [0] => address: 5678  parent: none
  //   [1] => address: 9999  parent: none
  EXPECT_EQ(2u, nodes->GetList().size());
  int id0 = GetNodeWithNameID(nodes, sid_5678);
  int id1 = GetNodeWithNameID(nodes, sid_9999);
  EXPECT_NE(-1, id0);
  EXPECT_NE(-1, id1);
  EXPECT_TRUE(IsBacktraceInList(nodes, id0, kNoParent));
  EXPECT_TRUE(IsBacktraceInList(nodes, id1, kNoParent));

  // Counts should be a list with one item. Items with |bt1| are filtered.
  // For |stack2|, there are two allocations of 1000 bytes. which is above the
  // 1500 bytes threshold. For |stack3|, there are 1001 allocations of 1 bytes,
  // which is above the 1000 allocations threshold.
  const base::Value* backtraces =
      heaps_v2->FindPath({"allocators", "malloc", "nodes"});
  ASSERT_TRUE(backtraces);
  EXPECT_EQ(2u, backtraces->GetList().size());

  int node_bt2 = GetOffsetForBacktraceID(backtraces, id0);
  int node_bt3 = GetOffsetForBacktraceID(backtraces, id1);
  EXPECT_NE(-1, node_bt2);
  EXPECT_NE(-1, node_bt3);
}

TEST(ProfilingJsonExporterTest, MemoryMaps) {
  AllocationEventSet events;
  ExportParams params;
  params.maps = memory_instrumentation::OSMetrics::GetProcessMemoryMaps(
      base::Process::Current().Pid());
  ASSERT_GT(params.maps.size(), 2u);

  std::ostringstream stream;

  params.allocs = AllocationEventSetToCountMap(events);
  params.min_size_threshold = kNoSizeThreshold;
  params.min_count_threshold = kNoCountThreshold;
  ExportMemoryMapsAndV2StackTraceToJSON(&params, stream);
  std::string json = stream.str();

  // JSON should parse.
  base::JSONReader reader(base::JSON_PARSE_RFC);
  std::unique_ptr<base::Value> root = reader.ReadToValue(stream.str());
  ASSERT_EQ(base::JSONReader::JSON_NO_ERROR, reader.error_code())
      << reader.GetErrorMessage();
  ASSERT_TRUE(root);

  const base::Value* region = FindFirstRegionWithAnyName(root.get());
  ASSERT_TRUE(region) << "Array contains no named vm regions";

  const base::Value* start_address =
      region->FindKeyOfType("sa", base::Value::Type::STRING);
  ASSERT_TRUE(start_address);
  EXPECT_NE(start_address->GetString(), "");
  EXPECT_NE(start_address->GetString(), "0");

  const base::Value* size =
      region->FindKeyOfType("sz", base::Value::Type::STRING);
  ASSERT_TRUE(size);
  EXPECT_NE(size->GetString(), "");
  EXPECT_NE(size->GetString(), "0");
}

TEST(ProfilingJsonExporterTest, Context) {
  BacktraceStorage backtrace_storage;
  ExportParams params;

  std::vector<Address> stack;
  stack.push_back(Address(0x1234));
  const Backtrace* bt = backtrace_storage.Insert(std::move(stack));

  std::string context_str1("Context 1");
  int context_id1 = 1;
  params.context_map[context_str1] = context_id1;
  std::string context_str2("Context 2");
  int context_id2 = 2;
  params.context_map[context_str2] = context_id2;

  // Make 4 events, all with identical metadata except context. Two share the
  // same context so should get folded, one has unique context, and one has no
  // context.
  AllocationEventSet events;
  events.insert(AllocationEvent(AllocatorType::kPartitionAlloc, Address(0x1),
                                16, bt, context_id1));
  events.insert(AllocationEvent(AllocatorType::kPartitionAlloc, Address(0x2),
                                16, bt, context_id2));
  events.insert(
      AllocationEvent(AllocatorType::kPartitionAlloc, Address(0x3), 16, bt, 0));
  events.insert(AllocationEvent(AllocatorType::kPartitionAlloc, Address(0x4),
                                16, bt, context_id1));

  std::ostringstream stream;

  params.allocs = AllocationEventSetToCountMap(events);
  params.min_size_threshold = kNoSizeThreshold;
  params.min_count_threshold = kNoCountThreshold;
  ExportMemoryMapsAndV2StackTraceToJSON(&params, stream);
  std::string json = stream.str();

  // JSON should parse.
  base::JSONReader reader(base::JSON_PARSE_RFC);
  std::unique_ptr<base::Value> root = reader.ReadToValue(stream.str());
  ASSERT_EQ(base::JSONReader::JSON_NO_ERROR, reader.error_code())
      << reader.GetErrorMessage();
  ASSERT_TRUE(root);

  // Retrieve the allocations.
  const base::Value* heaps_v2 = root->FindKey("heaps_v2");
  ASSERT_TRUE(heaps_v2);

  const base::Value* counts =
      heaps_v2->FindPath({"allocators", "partition_alloc", "counts"});
  ASSERT_TRUE(counts);
  const base::Value* types =
      heaps_v2->FindPath({"allocators", "partition_alloc", "types"});
  ASSERT_TRUE(types);

  const auto& counts_list = counts->GetList();
  const auto& types_list = types->GetList();

  // There should be three allocations, two coalesced ones, one with unique
  // context, and one with no context.
  EXPECT_EQ(3u, counts_list.size());
  EXPECT_EQ(3u, types_list.size());

  const base::Value* types_map = heaps_v2->FindPath({"maps", "types"});
  ASSERT_TRUE(types_map);
  const base::Value* strings = heaps_v2->FindPath({"maps", "strings"});
  ASSERT_TRUE(strings);

  // Reconstruct the map from type id to string.
  std::map<int, std::string> type_to_string;
  for (const auto& type : types_map->GetList()) {
    const base::Value* id =
        type.FindKeyOfType("id", base::Value::Type::INTEGER);
    ASSERT_TRUE(id);
    const base::Value* name_sid =
        type.FindKeyOfType("name_sid", base::Value::Type::INTEGER);
    ASSERT_TRUE(name_sid);

    type_to_string[id->GetInt()] =
        GetStringFromStringTable(strings, name_sid->GetInt());
  }

  // Track the three entries we have down to what we expect. The order is not
  // defined so this is relatively complex to do.
  bool found_double_context = false;  // Allocations sharing the same context.
  bool found_single_context = false;  // Allocation with unique context.
  bool found_no_context = false;      // Allocation with no context.
  for (size_t i = 0; i < types_list.size(); i++) {
    const auto& found = type_to_string.find(types_list[i].GetInt());
    ASSERT_NE(type_to_string.end(), found);
    if (found->second == context_str1) {
      // Context string matches the one with two allocations.
      ASSERT_FALSE(found_double_context);
      found_double_context = true;
      ASSERT_EQ(2, counts_list[i].GetInt());
    } else if (found->second == context_str2) {
      // Context string matches the one with one allocation.
      ASSERT_FALSE(found_single_context);
      found_single_context = true;
      ASSERT_EQ(1, counts_list[i].GetInt());
    } else if (found->second == "[unknown]") {
      // Context string for the one with no context.
      ASSERT_FALSE(found_no_context);
      found_no_context = true;
      ASSERT_EQ(1, counts_list[i].GetInt());
    }
  }

  // All three types of things should have been found in the loop.
  ASSERT_TRUE(found_double_context);
  ASSERT_TRUE(found_single_context);
  ASSERT_TRUE(found_no_context);
}

}  // namespace heap_profiling
