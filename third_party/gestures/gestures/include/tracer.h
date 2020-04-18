// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "gestures/include/prop_registry.h"

#ifndef GESTURES_TRACER_H__
#define GESTURES_TRACER_H__

namespace gestures {

typedef void (*WriteFn)(const char*);

// This class will automatically help us manage tracing stuff.
// It has a X Property "Tracing Enabled". You can set it true to
// enable tracing.
// In the main program, you can simply use Trace function provided
// by this class to write tracing messages, and it will handle
// whether to output the message or not automatically.

class Tracer {
  FRIEND_TEST(TracerTest, TraceTest);
 public:
  Tracer(PropRegistry* prop_reg, WriteFn write_fn);
  ~Tracer() {};
  void Trace(const char* message, const char* name);

 private:
  WriteFn write_fn_;
  // Disable and enable tracing by setting false and true respectively
  BoolProperty tracing_enabled_;
};
}  // namespace gestures

#endif  // GESTURES_TRACER_H__
