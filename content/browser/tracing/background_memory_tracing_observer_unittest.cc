// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/background_memory_tracing_observer.h"

#include "base/allocator/buildflags.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/trace_event/heap_profiler_allocation_context_tracker.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_manager_test_utils.h"
#include "base/trace_event/trace_log.h"
#include "build/build_config.h"
#include "content/browser/tracing/background_tracing_config_impl.h"
#include "content/public/test/test_browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::trace_event::AllocationContextTracker;
using base::trace_event::MemoryDumpManager;
using base::trace_event::TraceConfig;
using base::trace_event::TraceLog;
using base::trace_event::InitializeMemoryDumpManagerForInProcessTesting;

namespace content {
namespace {

std::unique_ptr<BackgroundTracingConfigImpl> ReadFromJSONString(
    const std::string& json_text) {
  std::unique_ptr<base::Value> json_value(base::JSONReader::Read(json_text));

  base::DictionaryValue* dict = nullptr;
  if (json_value)
    json_value->GetAsDictionary(&dict);

  std::unique_ptr<BackgroundTracingConfigImpl> config(
      static_cast<BackgroundTracingConfigImpl*>(
          BackgroundTracingConfig::FromDict(dict).release()));
  CHECK(config);
  return config;
}

}  // namespace

class BackgroundMemoryTracingObserverTest : public testing::Test {
 public:
  BackgroundMemoryTracingObserverTest()
      : ui_thread_(BrowserThread::UI, &message_loop_) {}

  void SetUp() override {
    mdm_ = MemoryDumpManager::CreateInstanceForTesting();
    InitializeMemoryDumpManagerForInProcessTesting(
        /*is_coordinator_process=*/false);
  }

  void TearDown() override { mdm_ = nullptr; }

 private:
  std::unique_ptr<MemoryDumpManager> mdm_;
  base::MessageLoop message_loop_;
  TestBrowserThread ui_thread_;
};

TEST_F(BackgroundMemoryTracingObserverTest, NoOpOnNonMemoryConfig) {
  auto* observer = BackgroundMemoryTracingObserver::GetInstance();
  auto config = ReadFromJSONString(
      "{\"mode\":\"REACTIVE_TRACING_MODE\", \"configs\": [{\"category\": "
      "\"BENCHMARK\", \"rule\": "
      "\"MONITOR_AND_DUMP_WHEN_SPECIFIC_HISTOGRAM_AND_VALUE\", "
      "\"histogram_name\":\"foo\", \"histogram_lower_value\": 1, "
      "\"histogram_upper_value\": 2}]}");
  observer->OnScenarioActivated(config.get());
  EXPECT_FALSE(observer->heap_profiling_enabled_for_testing());
  observer->OnTracingEnabled(BackgroundTracingConfigImpl::BENCHMARK);
  EXPECT_EQ(0u, TraceLog::GetInstance()->enabled_modes());
  EXPECT_EQ(AllocationContextTracker::CaptureMode::DISABLED,
            AllocationContextTracker::capture_mode());

  observer->OnScenarioAborted();
  EXPECT_FALSE(observer->heap_profiling_enabled_for_testing());
  EXPECT_EQ(0u, TraceLog::GetInstance()->enabled_modes());
  EXPECT_EQ(AllocationContextTracker::CaptureMode::DISABLED,
            AllocationContextTracker::capture_mode());
}

TEST_F(BackgroundMemoryTracingObserverTest, OnlyBackgroundDumpConfig) {
  auto* observer = BackgroundMemoryTracingObserver::GetInstance();
  auto config = ReadFromJSONString(
      "{\"mode\":\"REACTIVE_TRACING_MODE\", \"configs\": [{\"category\": "
      "\"BENCHMARK_MEMORY_LIGHT\", \"rule\": "
      "\"MONITOR_AND_DUMP_WHEN_SPECIFIC_HISTOGRAM_AND_VALUE\", "
      "\"histogram_name\":\"foo\", \"histogram_lower_value\": 1, "
      "\"histogram_upper_value\": 2}]}");
  observer->OnScenarioActivated(config.get());
  EXPECT_FALSE(observer->heap_profiling_enabled_for_testing());
  observer->OnTracingEnabled(BackgroundTracingConfigImpl::BENCHMARK);
  EXPECT_EQ(0u, TraceLog::GetInstance()->enabled_modes());
  EXPECT_EQ(AllocationContextTracker::CaptureMode::DISABLED,
            AllocationContextTracker::capture_mode());

  observer->OnScenarioAborted();
  EXPECT_FALSE(observer->heap_profiling_enabled_for_testing());
  EXPECT_EQ(0u, TraceLog::GetInstance()->enabled_modes());
  EXPECT_EQ(AllocationContextTracker::CaptureMode::DISABLED,
            AllocationContextTracker::capture_mode());
}

TEST_F(BackgroundMemoryTracingObserverTest, DISABLED_HeapProfilingConfig) {
  auto* observer = BackgroundMemoryTracingObserver::GetInstance();
  auto config = ReadFromJSONString(
      "{\"mode\":\"REACTIVE_TRACING_MODE\", \"configs\": [{\"category\": "
      "\"BENCHMARK_MEMORY_LIGHT\", \"rule\": "
      "\"MONITOR_AND_DUMP_WHEN_SPECIFIC_HISTOGRAM_AND_VALUE\", "
      "\"histogram_name\":\"foo\", \"histogram_lower_value\": 1, "
      "\"histogram_upper_value\": 2, \"args\": {\"enable_heap_profiler_mode\": "
      "\"background\"}}]}");
  observer->OnScenarioActivated(config.get());
  EXPECT_TRUE(observer->heap_profiling_enabled_for_testing());
  observer->OnTracingEnabled(BackgroundTracingConfigImpl::BENCHMARK);
  EXPECT_EQ(0u, TraceLog::GetInstance()->enabled_modes());
#if BUILDFLAG(USE_ALLOCATOR_SHIM) && !defined(OS_NACL)
  EXPECT_EQ(AllocationContextTracker::CaptureMode::PSEUDO_STACK,
            AllocationContextTracker::capture_mode());
#else
  EXPECT_EQ(AllocationContextTracker::CaptureMode::DISABLED,
            AllocationContextTracker::capture_mode());
#endif

  observer->OnScenarioAborted();
  EXPECT_FALSE(observer->heap_profiling_enabled_for_testing());
  EXPECT_EQ(0u, TraceLog::GetInstance()->enabled_modes());
  EXPECT_EQ(AllocationContextTracker::CaptureMode::DISABLED,
            AllocationContextTracker::capture_mode());
}

TEST_F(BackgroundMemoryTracingObserverTest, DISABLED_HeapProfilingWithFilters) {
  auto* observer = BackgroundMemoryTracingObserver::GetInstance();
  auto config = ReadFromJSONString(
      "{\"mode\":\"REACTIVE_TRACING_MODE\", \"configs\": [{\"category\": "
      "\"BENCHMARK_MEMORY_LIGHT\", \"rule\": "
      "\"MONITOR_AND_DUMP_WHEN_SPECIFIC_HISTOGRAM_AND_VALUE\", "
      "\"histogram_name\":\"foo\", \"histogram_lower_value\": 1, "
      "\"histogram_upper_value\": 2, \"args\": {\"enable_heap_profiler_mode\": "
      "\"background\", \"heap_profiler_category_filter\": \"cat,dog\"}}]}");
  observer->OnScenarioActivated(config.get());
  EXPECT_TRUE(observer->heap_profiling_enabled_for_testing());
  observer->OnTracingEnabled(BackgroundTracingConfigImpl::BENCHMARK);
#if BUILDFLAG(USE_ALLOCATOR_SHIM) && !defined(OS_NACL)
  EXPECT_EQ(AllocationContextTracker::CaptureMode::PSEUDO_STACK,
            AllocationContextTracker::capture_mode());
  EXPECT_EQ(TraceLog::FILTERING_MODE, TraceLog::GetInstance()->enabled_modes());
  const char kExpectedConfig[] =
      "{\"filter_predicate\":\"heap_profiler_predicate\","
      "\"included_categories\":[\"cat\",\"dog\"]}";
  auto trace_config = TraceLog::GetInstance()->GetCurrentTraceConfig();
  ASSERT_EQ(1u, trace_config.event_filters().size());
  base::DictionaryValue filter_dict;
  trace_config.event_filters()[0].ToDict(&filter_dict);
  std::string filter_str;
  base::JSONWriter::Write(filter_dict, &filter_str);
  EXPECT_EQ(kExpectedConfig, filter_str);
#else   // BUILDFLAG(USE_ALLOCATOR_SHIM) && !defined(OS_NACL)
  EXPECT_EQ(0u, TraceLog::GetInstance()->enabled_modes());
  EXPECT_EQ(AllocationContextTracker::CaptureMode::DISABLED,
            AllocationContextTracker::capture_mode());
#endif  // BUILDFLAG(USE_ALLOCATOR_SHIM) && !defined(OS_NACL)

  observer->OnScenarioAborted();
  EXPECT_FALSE(observer->heap_profiling_enabled_for_testing());
  EXPECT_EQ(0u, TraceLog::GetInstance()->enabled_modes());
  EXPECT_EQ(AllocationContextTracker::CaptureMode::DISABLED,
            AllocationContextTracker::capture_mode());
}

}  // namespace content
