// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "perf_test_helpers.h"

#include <algorithm>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"

namespace tracing {

namespace {

void PrintPerfTestMs(const std::string& name, int64_t value) {
  CHECK(::testing::UnitTest::GetInstance() != nullptr) << "Must be GTest.";
  const ::testing::TestInfo* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  CHECK(test_info != nullptr) << "Must be GTest.";

  perf_test::PrintResult(test_info->test_case_name(),
                         std::string(".") + test_info->name(),
                         name, static_cast<double>(value), "ms", true);
}

}  // namespace

ScopedStopwatch::ScopedStopwatch(const std::string& name) : name_(name) {
  begin_= base::TimeTicks::Now();
}

ScopedStopwatch::~ScopedStopwatch() {
  int64_t value = (base::TimeTicks::Now() - begin_).InMilliseconds();
  PrintPerfTestMs(name_, value);
}

IterableStopwatch::IterableStopwatch(const std::string& name) : name_(name) {
  begin_ = base::TimeTicks::Now();
}

void IterableStopwatch::NextLap() {
  base::TimeTicks now = base::TimeTicks::Now();
  int64_t elapsed = (now - begin_).InMilliseconds();
  begin_ = now;
  laps_.push_back(elapsed);
}

IterableStopwatch::~IterableStopwatch() {
  CHECK(!laps_.empty());
  std::sort(laps_.begin(), laps_.end());
  int64_t median = laps_.at(laps_.size() / 2);
  PrintPerfTestMs(name_, median);
}

}  // namespace tracing
