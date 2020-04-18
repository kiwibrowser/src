// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <iterator>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/trace_event/heap_profiler_stack_frame_deduplicator.h"
#include "base/trace_event/trace_event_argument.h"
#include "perf_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tracing {
namespace {

using namespace base::trace_event;

class HeapProfilerPerfTest : public testing::Test {
 protected:
  struct StackFrameVariations {
    size_t skew;
    std::vector<StackFrame> values;
  };
  using BacktraceVariations = std::vector<StackFrameVariations>;

  BacktraceVariations GetRandomBacktraceVariations() {
    struct VariationSpec {
      size_t value_count;  // number of unique frames
      size_t skew;         // see GetSkewedRandom()
      size_t length;       // number of backtrace levels covered
    };
    static const VariationSpec specs[] = {
        // These values give realistic insertion pattern with 60% hit count.
        {100, 90, Backtrace::kMaxFrameCount - 5},
        {10, 5, 5}};

    BacktraceVariations backtrace_variations;
    const VariationSpec* spec = std::begin(specs);
    size_t spec_end_depth = spec->length;
    for (size_t depth = 0; depth != Backtrace::kMaxFrameCount; ++depth) {
      if (depth == spec_end_depth) {
        if (spec != std::end(specs) - 1) {
          spec++;
          spec_end_depth = depth + spec->length;
        } else {
          spec_end_depth = Backtrace::kMaxFrameCount;
        }
      }
      backtrace_variations.push_back({spec->skew});
      auto& frame_variations = backtrace_variations.back();
      for (size_t v = 0; v != spec->value_count; ++v) {
        uintptr_t pc = static_cast<uintptr_t>(GetRandom64());
        frame_variations.values.push_back(
            StackFrame::FromProgramCounter(reinterpret_cast<void*>(pc)));
      }
    }

    return backtrace_variations;
  }

  size_t InsertRandomBacktraces(StackFrameDeduplicator* deduplicator,
                                const BacktraceVariations& backtrace_variations,
                                size_t max_frame_count) {
    size_t insert_count = 0;
    while (true) {
      insert_count++;
      Backtrace backtrace = RandomBacktrace(backtrace_variations);
      deduplicator->Insert(backtrace.frames,
                           backtrace.frames + backtrace.frame_count);
      size_t frame_count = deduplicator->end() - deduplicator->begin();
      if (frame_count > max_frame_count) {
        break;
      }
    }
    return insert_count;
  }

 private:
  uint32_t GetRandom32() {
    random_seed_ = random_seed_ * 1664525 + 1013904223;
    return random_seed_;
  }

  uint64_t GetRandom64() {
    uint64_t rnd = GetRandom32();
    return (rnd << 32) | GetRandom32();
  }

  // Returns random number in range [0, max).
  uint32_t GetRandom32(uint32_t max) {
    uint64_t rnd = GetRandom32();
    return static_cast<uint32_t>((rnd * max) >> 32);
  }

  // In 1/skew cases returns random value in range [0, max), in all
  // other cases returns value in range [0, max/skew). The average value
  // ends up being (2*skew - 1) / (2*skew^2) * max.
  uint32_t GetSkewedRandom(uint32_t max, uint32_t skew) {
    if (GetRandom32(skew) != (skew - 1)) {
      max /= skew;
    }
    return GetRandom32(max);
  }

  Backtrace RandomBacktrace(const BacktraceVariations& backtrace_variations) {
    Backtrace backtrace;
    backtrace.frame_count = Backtrace::kMaxFrameCount;
    for (size_t depth = 0; depth != backtrace.frame_count; ++depth) {
      const auto& frame_variations = backtrace_variations[depth];
      size_t value_index =
          GetSkewedRandom(static_cast<uint32_t>(frame_variations.values.size()),
                          static_cast<uint32_t>(frame_variations.skew));
      backtrace.frames[depth] = frame_variations.values[value_index];
    }
    return backtrace;
  }

 private:
  uint32_t random_seed_ = 777;
};

TEST_F(HeapProfilerPerfTest, DeduplicateStackFrames) {
  StackFrameDeduplicator deduplicator;

  auto variations = GetRandomBacktraceVariations();

  {
    ScopedStopwatch stopwatch("time_to_insert");
    InsertRandomBacktraces(&deduplicator, variations, 1000000);
  }
}

TEST_F(HeapProfilerPerfTest, AppendStackFramesAsTraceFormat) {
  StackFrameDeduplicator deduplicator;

  auto variations = GetRandomBacktraceVariations();
  InsertRandomBacktraces(&deduplicator, variations, 1000000);

  {
    ScopedStopwatch stopwatch("time_to_append");
    std::string json;
    deduplicator.AppendAsTraceFormat(&json);
  }
}

}  // namespace
}  // namespace tracing
