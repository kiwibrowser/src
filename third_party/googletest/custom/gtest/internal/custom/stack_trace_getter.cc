// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/googletest/custom/gtest/internal/custom/stack_trace_getter.h"

#include "base/debug/stack_trace.h"
#include "base/format_macros.h"

namespace {

// The number of frames on the top of the stack to ignore in UponLeavingGTest.
// This may vary from build to build and platform to platform. On Windows
// release builds at the time of writing, the top two frames should be skipped:
//   base::debug::StackTrace::StackTrace
//   StackTraceGetter::UponLeavingGTest
enum : size_t { kDepartureSkipFrames = 2 };

}  // namespace

std::string StackTraceGetter::CurrentStackTrace(int max_depth, int skip_count) {
  base::debug::StackTrace stack_trace;

  size_t frame_count = 0;
  const void* const* addresses = stack_trace.Addresses(&frame_count);

  // Drop the frames at the tail that were present the last time gtest was left.
  if (frame_count_upon_leaving_gtest_ &&
      frame_count > frame_count_upon_leaving_gtest_) {
    frame_count -= frame_count_upon_leaving_gtest_;
  }

  // Ignore frames to skip.
  if (skip_count >= 0 && static_cast<size_t>(skip_count) < frame_count) {
    frame_count -= skip_count;
    addresses += skip_count;
  }

  // Only return as many as requested.
  if (max_depth >= 0 && static_cast<size_t>(max_depth) < frame_count)
    frame_count = static_cast<size_t>(max_depth);

  return base::debug::StackTrace(addresses, frame_count).ToString();
}

void StackTraceGetter::UponLeavingGTest() {
  // Remember how deep the stack is as gtest is left.
  base::debug::StackTrace().Addresses(&frame_count_upon_leaving_gtest_);
  if (frame_count_upon_leaving_gtest_ > kDepartureSkipFrames)
    frame_count_upon_leaving_gtest_ -= kDepartureSkipFrames;
}
