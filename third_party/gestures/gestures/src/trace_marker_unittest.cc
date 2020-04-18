// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include <gtest/gtest.h>

#include "gestures/include/trace_marker.h"

namespace gestures {

class TraceMarkerTest : public ::testing::Test {};

TEST(TraceMarkerTest, DeleteTraceMarkerTest) {
    EXPECT_EQ(NULL, TraceMarker::GetTraceMarker());
    TraceMarker::CreateTraceMarker();
    EXPECT_TRUE(NULL != TraceMarker::GetTraceMarker());
    TraceMarker::DeleteTraceMarker();
    EXPECT_EQ(NULL, TraceMarker::GetTraceMarker());
};
}  // namespace gestures
