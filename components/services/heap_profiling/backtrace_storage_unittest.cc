// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/backtrace_storage.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace heap_profiling {

TEST(BacktraceStorage, KeyStability) {
  BacktraceStorage storage;

  // Make a lot of unique backtraces to force reallocation of the hash table
  // several times.
  const size_t num_traces = 1000;
  std::vector<const Backtrace*> traces;
  for (size_t i = 0; i < num_traces; i++) {
    // Each backtrace should contain its index as the only stack entry.
    std::vector<Address> addrs;
    addrs.push_back(Address(i));
    traces.push_back(storage.Insert(std::move(addrs)));
  }

  // Validate the backtraces are still valid.
  for (size_t i = 0; i < num_traces; i++) {
    ASSERT_EQ(1u, traces[i]->addrs().size());
    EXPECT_EQ(Address(i), traces[i]->addrs()[0]);
  }
}

}  // namespace heap_profiling
