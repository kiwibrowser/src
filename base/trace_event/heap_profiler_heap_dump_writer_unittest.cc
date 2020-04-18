// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/trace_event/heap_profiler_heap_dump_writer.h"

#include <stddef.h>

#include <memory>
#include <set>
#include <string>

#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/trace_event/heap_profiler_allocation_context.h"
#include "base/trace_event/heap_profiler_stack_frame_deduplicator.h"
#include "base/trace_event/heap_profiler_type_name_deduplicator.h"
#include "base/trace_event/trace_event_argument.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using base::trace_event::StackFrame;

// Define all strings once, because the deduplicator requires pointer equality,
// and string interning is unreliable.
StackFrame kBrowserMain = StackFrame::FromTraceEventName("BrowserMain");
StackFrame kRendererMain = StackFrame::FromTraceEventName("RendererMain");
StackFrame kCreateWidget = StackFrame::FromTraceEventName("CreateWidget");
StackFrame kInitialize = StackFrame::FromTraceEventName("Initialize");
StackFrame kGetBitmap = StackFrame::FromTraceEventName("GetBitmap");

const char kInt[] = "int";
const char kBool[] = "bool";
const char kString[] = "string";

}  // namespace

namespace base {
namespace trace_event {
namespace internal {

std::unique_ptr<const Value> WriteAndReadBack(const std::set<Entry>& entries) {
  std::unique_ptr<TracedValue> traced_value = Serialize(entries);
  std::string json;
  traced_value->AppendAsTraceFormat(&json);
  return JSONReader::Read(json);
}

std::unique_ptr<const DictionaryValue> WriteAndReadBackEntry(Entry entry) {
  std::set<Entry> input_entries;
  input_entries.insert(entry);

  std::unique_ptr<const Value> json_dict = WriteAndReadBack(input_entries);

  // Note: Ideally these should use |ASSERT_TRUE| instead of |EXPECT_TRUE|, but
  // |ASSERT_TRUE| can only be used in void functions.
  const DictionaryValue* dictionary;
  EXPECT_TRUE(json_dict->GetAsDictionary(&dictionary));

  const ListValue* json_entries;
  EXPECT_TRUE(dictionary->GetList("entries", &json_entries));

  const DictionaryValue* json_entry;
  EXPECT_TRUE(json_entries->GetDictionary(0, &json_entry));

  return json_entry->CreateDeepCopy();
}

// Given a desired stack frame ID and type ID, looks up the entry in the set and
// asserts that it is present and has the expected size and count.
void AssertSizeAndCountEq(const std::set<Entry>& entries,
                          int stack_frame_id,
                          int type_id,
                          const AllocationMetrics& expected) {
  // The comparison operator for |Entry| does not take size into account, so by
  // setting only stack frame ID and type ID, the real entry can be found.
  Entry entry;
  entry.stack_frame_id = stack_frame_id;
  entry.type_id = type_id;
  auto it = entries.find(entry);

  ASSERT_NE(entries.end(), it) << "No entry found for sf = " << stack_frame_id
                               << ", type = " << type_id << ".";
  ASSERT_EQ(expected.size, it->size) << "Wrong size for sf = " << stack_frame_id
                                     << ", type = " << type_id << ".";
  ASSERT_EQ(expected.count, it->count)
      << "Wrong count for sf = " << stack_frame_id << ", type = " << type_id
      << ".";
}

// Given a desired stack frame ID and type ID, asserts that no entry was dumped
// for that that particular combination of stack frame and type.
void AssertNotDumped(const std::set<Entry>& entries,
                     int stack_frame_id,
                     int type_id) {
  // The comparison operator for |Entry| does not take size into account, so by
  // setting only stack frame ID and type ID, the real entry can be found.
  Entry entry;
  entry.stack_frame_id = stack_frame_id;
  entry.type_id = type_id;
  auto it = entries.find(entry);
  ASSERT_EQ(entries.end(), it)
      << "Entry should not be present for sf = " << stack_frame_id
      << ", type = " << type_id << ".";
}

TEST(HeapDumpWriterTest, BacktraceIndex) {
  Entry entry;
  entry.stack_frame_id = -1;  // -1 means empty backtrace.
  entry.type_id = 0;
  entry.size = 1;
  entry.count = 1;

  std::unique_ptr<const DictionaryValue> json_entry =
      WriteAndReadBackEntry(entry);

  // For an empty backtrace, the "bt" key cannot reference a stack frame.
  // Instead it should be set to the empty string.
  std::string backtrace_index;
  ASSERT_TRUE(json_entry->GetString("bt", &backtrace_index));
  ASSERT_EQ("", backtrace_index);

  // Also verify that a non-negative backtrace index is dumped properly.
  entry.stack_frame_id = 2;
  json_entry = WriteAndReadBackEntry(entry);
  ASSERT_TRUE(json_entry->GetString("bt", &backtrace_index));
  ASSERT_EQ("2", backtrace_index);
}

TEST(HeapDumpWriterTest, TypeId) {
  Entry entry;
  entry.type_id = -1;  // -1 means sum over all types.
  entry.stack_frame_id = 0;
  entry.size = 1;
  entry.count = 1;

  std::unique_ptr<const DictionaryValue> json_entry =
      WriteAndReadBackEntry(entry);

  // Entries for the cumulative size of all types should not have the "type"
  // key set.
  ASSERT_FALSE(json_entry->HasKey("type"));

  // Also verify that a non-negative type ID is dumped properly.
  entry.type_id = 2;
  json_entry = WriteAndReadBackEntry(entry);
  std::string type_id;
  ASSERT_TRUE(json_entry->GetString("type", &type_id));
  ASSERT_EQ("2", type_id);
}

TEST(HeapDumpWriterTest, SizeAndCountAreHexadecimal) {
  // Take a number between 2^63 and 2^64 (or between 2^31 and 2^32 if |size_t|
  // is not 64 bits).
  const size_t large_value =
      sizeof(size_t) == 8 ? 0xffffffffffffffc5 : 0xffffff9d;
  const char* large_value_str =
      sizeof(size_t) == 8 ? "ffffffffffffffc5" : "ffffff9d";
  Entry entry;
  entry.type_id = 0;
  entry.stack_frame_id = 0;
  entry.size = large_value;
  entry.count = large_value;

  std::unique_ptr<const DictionaryValue> json_entry =
      WriteAndReadBackEntry(entry);

  std::string size;
  ASSERT_TRUE(json_entry->GetString("size", &size));
  ASSERT_EQ(large_value_str, size);

  std::string count;
  ASSERT_TRUE(json_entry->GetString("count", &count));
  ASSERT_EQ(large_value_str, count);
}

TEST(HeapDumpWriterTest, BacktraceTypeNameTable) {
  std::unordered_map<AllocationContext, AllocationMetrics> metrics_by_context;

  AllocationContext ctx;
  ctx.backtrace.frames[0] = kBrowserMain;
  ctx.backtrace.frames[1] = kCreateWidget;
  ctx.backtrace.frame_count = 2;
  ctx.type_name = kInt;

  // 10 bytes with context { type: int, bt: [BrowserMain, CreateWidget] }.
  metrics_by_context[ctx] = {10, 5};

  ctx.type_name = kBool;

  // 18 bytes with context { type: bool, bt: [BrowserMain, CreateWidget] }.
  metrics_by_context[ctx] = {18, 18};

  ctx.backtrace.frames[0] = kRendererMain;
  ctx.backtrace.frames[1] = kInitialize;
  ctx.backtrace.frame_count = 2;

  // 30 bytes with context { type: bool, bt: [RendererMain, Initialize] }.
  metrics_by_context[ctx] = {30, 30};

  ctx.type_name = kString;

  // 19 bytes with context { type: string, bt: [RendererMain, Initialize] }.
  metrics_by_context[ctx] = {19, 4};

  // At this point the heap looks like this:
  //
  // |        | CrWidget <- BrMain | Init <- RenMain |     Sum     |
  // +--------+--------------------+-----------------+-------------+
  // |        |       size   count |   size    count | size  count |
  // | int    |         10       5 |      0        0 |   10      5 |
  // | bool   |         18      18 |     30       30 |   48     48 |
  // | string |          0       0 |     19        4 |   19      4 |
  // +--------+--------------------+-----------------+-------------+
  // | Sum    |         28      23 |     49      34  |   77     57 |

  auto stack_frame_deduplicator = WrapUnique(new StackFrameDeduplicator);
  auto type_name_deduplicator = WrapUnique(new TypeNameDeduplicator);
  HeapDumpWriter writer(stack_frame_deduplicator.get(),
                        type_name_deduplicator.get(), 10u);
  const std::set<Entry>& dump = writer.Summarize(metrics_by_context);

  // Get the indices of the backtraces and types by adding them again to the
  // deduplicator. Because they were added before, the same number is returned.
  StackFrame bt0[] = {kRendererMain, kInitialize};
  StackFrame bt1[] = {kBrowserMain, kCreateWidget};
  int bt_renderer_main = stack_frame_deduplicator->Insert(bt0, bt0 + 1);
  int bt_browser_main = stack_frame_deduplicator->Insert(bt1, bt1 + 1);
  int bt_renderer_main_initialize =
      stack_frame_deduplicator->Insert(bt0, bt0 + 2);
  int bt_browser_main_create_widget =
      stack_frame_deduplicator->Insert(bt1, bt1 + 2);
  int type_id_int = type_name_deduplicator->Insert(kInt);
  int type_id_bool = type_name_deduplicator->Insert(kBool);
  int type_id_string = type_name_deduplicator->Insert(kString);

  // Full heap should have size 77.
  AssertSizeAndCountEq(dump, -1, -1, {77, 57});

  // 49 bytes in 34 chunks were allocated in RendererMain and children. Also
  // check the type breakdown.
  AssertSizeAndCountEq(dump, bt_renderer_main, -1, {49, 34});
  AssertSizeAndCountEq(dump, bt_renderer_main, type_id_bool, {30, 30});
  AssertSizeAndCountEq(dump, bt_renderer_main, type_id_string, {19, 4});

  // 28 bytes in 23 chunks were allocated in BrowserMain and children. Also
  // check the type breakdown.
  AssertSizeAndCountEq(dump, bt_browser_main, -1, {28, 23});
  AssertSizeAndCountEq(dump, bt_browser_main, type_id_int, {10, 5});
  AssertSizeAndCountEq(dump, bt_browser_main, type_id_bool, {18, 18});

  // In this test all bytes are allocated in leaf nodes, so check again one
  // level deeper.
  AssertSizeAndCountEq(dump, bt_renderer_main_initialize, -1, {49, 34});
  AssertSizeAndCountEq(dump, bt_renderer_main_initialize, type_id_bool,
                       {30, 30});
  AssertSizeAndCountEq(dump, bt_renderer_main_initialize, type_id_string,
                       {19, 4});
  AssertSizeAndCountEq(dump, bt_browser_main_create_widget, -1, {28, 23});
  AssertSizeAndCountEq(dump, bt_browser_main_create_widget, type_id_int,
                       {10, 5});
  AssertSizeAndCountEq(dump, bt_browser_main_create_widget, type_id_bool,
                       {18, 18});

  // The type breakdown of the entrie heap should have been dumped as well.
  AssertSizeAndCountEq(dump, -1, type_id_int, {10, 5});
  AssertSizeAndCountEq(dump, -1, type_id_bool, {48, 48});
  AssertSizeAndCountEq(dump, -1, type_id_string, {19, 4});
}

TEST(HeapDumpWriterTest, InsignificantValuesNotDumped) {
  std::unordered_map<AllocationContext, AllocationMetrics> metrics_by_context;

  AllocationContext ctx;
  ctx.backtrace.frames[0] = kBrowserMain;
  ctx.backtrace.frames[1] = kCreateWidget;
  ctx.backtrace.frame_count = 2;

  // 0.5 KiB and 1 chunk in BrowserMain -> CreateWidget itself.
  metrics_by_context[ctx] = {512, 1};

  // 1 MiB and 1 chunk in BrowserMain -> CreateWidget -> GetBitmap.
  ctx.backtrace.frames[2] = kGetBitmap;
  ctx.backtrace.frame_count = 3;
  metrics_by_context[ctx] = {1024 * 1024, 1};

  // 400B and 1 chunk in BrowserMain -> CreateWidget -> Initialize.
  ctx.backtrace.frames[2] = kInitialize;
  ctx.backtrace.frame_count = 3;
  metrics_by_context[ctx] = {400, 1};

  auto stack_frame_deduplicator = WrapUnique(new StackFrameDeduplicator);
  auto type_name_deduplicator = WrapUnique(new TypeNameDeduplicator);
  HeapDumpWriter writer(stack_frame_deduplicator.get(),
                        type_name_deduplicator.get(), 512u);
  const std::set<Entry>& dump = writer.Summarize(metrics_by_context);

  // Get the indices of the backtraces and types by adding them again to the
  // deduplicator. Because they were added before, the same number is returned.
  StackFrame bt0[] = {kBrowserMain, kCreateWidget, kGetBitmap};
  StackFrame bt1[] = {kBrowserMain, kCreateWidget, kInitialize};
  int bt_browser_main = stack_frame_deduplicator->Insert(bt0, bt0 + 1);
  int bt_create_widget = stack_frame_deduplicator->Insert(bt0, bt0 + 2);
  int bt_get_bitmap = stack_frame_deduplicator->Insert(bt0, bt0 + 3);
  int bt_initialize = stack_frame_deduplicator->Insert(bt1, bt1 + 3);

  // Full heap should have size of 1 MiB + .9 KiB and 3 chunks.
  AssertSizeAndCountEq(dump, -1, -1 /* No type specified */,
                       {1024 * 1024 + 512 + 400, 3});

  // |GetBitmap| allocated 1 MiB and 1 chunk.
  AssertSizeAndCountEq(dump, bt_get_bitmap, -1, {1024 * 1024, 1});

  // Because |GetBitmap| was dumped, all of its parent nodes should have been
  // dumped too. |CreateWidget| has 1 MiB in |GetBitmap|, 400 bytes in
  // |Initialize|, and 512 bytes of its own and each in 1 chunk.
  AssertSizeAndCountEq(dump, bt_create_widget, -1,
                       {1024 * 1024 + 400 + 512, 3});
  AssertSizeAndCountEq(dump, bt_browser_main, -1, {1024 * 1024 + 400 + 512, 3});

  // Initialize was not significant, it should not have been dumped.
  AssertNotDumped(dump, bt_initialize, -1);
}

}  // namespace internal
}  // namespace trace_event
}  // namespace base
