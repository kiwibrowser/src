// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include <gtest/gtest.h>

#include "gestures/include/tracer.h"

using std::string;

namespace gestures {

class TracerTest : public ::testing::Test {};

class TraceMarkerMock {
 public:
  static string msg_written;
  static void StaticTraceWrite(const char* str) {
    msg_written = string(str);
  }
};
string TraceMarkerMock::msg_written = "";

TEST(TracerTest, TraceTest) {
  PropRegistry prop_reg;
  Tracer tracer(&prop_reg, TraceMarkerMock::StaticTraceWrite);
  tracer.Trace("TestMessage: ", "name");
  EXPECT_STREQ("", TraceMarkerMock::msg_written.c_str());
  tracer.tracing_enabled_.val_ = 1;
  tracer.Trace("TestMessage: ", "name");
  EXPECT_STREQ("TestMessage: name", TraceMarkerMock::msg_written.c_str());
  tracer.tracing_enabled_.val_ = 0;
  tracer.Trace("TestMessageNoUse: ", "name");
  EXPECT_STREQ("TestMessage: name", TraceMarkerMock::msg_written.c_str());
}
}  // namespace gestures
