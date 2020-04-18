// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/trace_event/heap_profiler_stack_frame_deduplicator.h"

#include <iterator>
#include <memory>

#include "base/macros.h"
#include "base/trace_event/heap_profiler_allocation_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace trace_event {

// Define all strings once, because the deduplicator requires pointer equality,
// and string interning is unreliable.
StackFrame kBrowserMain = StackFrame::FromTraceEventName("BrowserMain");
StackFrame kRendererMain = StackFrame::FromTraceEventName("RendererMain");
StackFrame kCreateWidget = StackFrame::FromTraceEventName("CreateWidget");
StackFrame kInitialize = StackFrame::FromTraceEventName("Initialize");
StackFrame kMalloc = StackFrame::FromTraceEventName("malloc");

TEST(StackFrameDeduplicatorTest, SingleBacktrace) {
  StackFrame bt[] = {kBrowserMain, kCreateWidget, kMalloc};

  // The call tree should look like this (index in brackets).
  //
  // BrowserMain [0]
  //   CreateWidget [1]
  //     malloc [2]

  std::unique_ptr<StackFrameDeduplicator> dedup(new StackFrameDeduplicator);
  ASSERT_EQ(2, dedup->Insert(std::begin(bt), std::end(bt)));

  auto iter = dedup->begin();
  ASSERT_EQ(kBrowserMain, (iter + 0)->frame);
  ASSERT_EQ(-1, (iter + 0)->parent_frame_index);

  ASSERT_EQ(kCreateWidget, (iter + 1)->frame);
  ASSERT_EQ(0, (iter + 1)->parent_frame_index);

  ASSERT_EQ(kMalloc, (iter + 2)->frame);
  ASSERT_EQ(1, (iter + 2)->parent_frame_index);

  ASSERT_TRUE(iter + 3 == dedup->end());
}

TEST(StackFrameDeduplicatorTest, SingleBacktraceWithNull) {
  StackFrame null_frame = StackFrame::FromTraceEventName(nullptr);
  StackFrame bt[] = {kBrowserMain, null_frame, kMalloc};

  // Deduplicator doesn't care about what's inside StackFrames,
  // and handles nullptr StackFrame values as any other.
  //
  // So the call tree should look like this (index in brackets).
  //
  // BrowserMain [0]
  //   (null) [1]
  //     malloc [2]

  std::unique_ptr<StackFrameDeduplicator> dedup(new StackFrameDeduplicator);
  ASSERT_EQ(2, dedup->Insert(std::begin(bt), std::end(bt)));

  auto iter = dedup->begin();
  ASSERT_EQ(kBrowserMain, (iter + 0)->frame);
  ASSERT_EQ(-1, (iter + 0)->parent_frame_index);

  ASSERT_EQ(null_frame, (iter + 1)->frame);
  ASSERT_EQ(0, (iter + 1)->parent_frame_index);

  ASSERT_EQ(kMalloc, (iter + 2)->frame);
  ASSERT_EQ(1, (iter + 2)->parent_frame_index);

  ASSERT_TRUE(iter + 3 == dedup->end());
}

// Test that there can be different call trees (there can be multiple bottom
// frames). Also verify that frames with the same name but a different caller
// are represented as distinct nodes.
TEST(StackFrameDeduplicatorTest, MultipleRoots) {
  StackFrame bt0[] = {kBrowserMain, kCreateWidget};
  StackFrame bt1[] = {kRendererMain, kCreateWidget};

  // The call tree should look like this (index in brackets).
  //
  // BrowserMain [0]
  //   CreateWidget [1]
  // RendererMain [2]
  //   CreateWidget [3]
  //
  // Note that there will be two instances of CreateWidget,
  // with different parents.

  std::unique_ptr<StackFrameDeduplicator> dedup(new StackFrameDeduplicator);
  ASSERT_EQ(1, dedup->Insert(std::begin(bt0), std::end(bt0)));
  ASSERT_EQ(3, dedup->Insert(std::begin(bt1), std::end(bt1)));

  auto iter = dedup->begin();
  ASSERT_EQ(kBrowserMain, (iter + 0)->frame);
  ASSERT_EQ(-1, (iter + 0)->parent_frame_index);

  ASSERT_EQ(kCreateWidget, (iter + 1)->frame);
  ASSERT_EQ(0, (iter + 1)->parent_frame_index);

  ASSERT_EQ(kRendererMain, (iter + 2)->frame);
  ASSERT_EQ(-1, (iter + 2)->parent_frame_index);

  ASSERT_EQ(kCreateWidget, (iter + 3)->frame);
  ASSERT_EQ(2, (iter + 3)->parent_frame_index);

  ASSERT_TRUE(iter + 4 == dedup->end());
}

TEST(StackFrameDeduplicatorTest, Deduplication) {
  StackFrame bt0[] = {kBrowserMain, kCreateWidget};
  StackFrame bt1[] = {kBrowserMain, kInitialize};

  // The call tree should look like this (index in brackets).
  //
  // BrowserMain [0]
  //   CreateWidget [1]
  //   Initialize [2]
  //
  // Note that BrowserMain will be re-used.

  std::unique_ptr<StackFrameDeduplicator> dedup(new StackFrameDeduplicator);
  ASSERT_EQ(1, dedup->Insert(std::begin(bt0), std::end(bt0)));
  ASSERT_EQ(2, dedup->Insert(std::begin(bt1), std::end(bt1)));

  auto iter = dedup->begin();
  ASSERT_EQ(kBrowserMain, (iter + 0)->frame);
  ASSERT_EQ(-1, (iter + 0)->parent_frame_index);

  ASSERT_EQ(kCreateWidget, (iter + 1)->frame);
  ASSERT_EQ(0, (iter + 1)->parent_frame_index);

  ASSERT_EQ(kInitialize, (iter + 2)->frame);
  ASSERT_EQ(0, (iter + 2)->parent_frame_index);

  ASSERT_TRUE(iter + 3 == dedup->end());

  // Inserting the same backtrace again should return the index of the existing
  // node.
  ASSERT_EQ(1, dedup->Insert(std::begin(bt0), std::end(bt0)));
  ASSERT_EQ(2, dedup->Insert(std::begin(bt1), std::end(bt1)));
  ASSERT_TRUE(dedup->begin() + 3 == dedup->end());
}

}  // namespace trace_event
}  // namespace base
