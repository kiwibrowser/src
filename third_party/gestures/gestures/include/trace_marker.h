// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <linux/limits.h>

#include "gestures/include/macros.h"

#ifndef GESTURES_TRACE_MARKER_H__
#define GESTURES_TRACE_MARKER_H__

#define TRACE_WRITE(x) TraceMarker::StaticTraceWrite(x)

namespace gestures {
// This class is used for writing message into the debugfs tracing system
// based on Ftrace provided by linux.
// By default, the debugfs is mounted at /sys/kernel/debug/
// It will automatically help you handle the detail things. If you want to
// write a message into tracing system, you can simply use
// TRACE_WRITE("MESSAGE") and you can find the message appears in the file
// debugfs/tracing/trace

class TraceMarker {
 public:
  static void CreateTraceMarker();
  static void DeleteTraceMarker();
  static void StaticTraceWrite(const char* str);
  static TraceMarker* GetTraceMarker();
  void TraceWrite(const char* str);

 private:
  TraceMarker();
  ~TraceMarker();
  DISALLOW_COPY_AND_ASSIGN(TraceMarker);
  static TraceMarker* trace_marker_;
  static int trace_marker_count_;
  int fd_;
  bool FindDebugfs(const char** ret) const;
  bool FindTraceMarker(char** ret) const;
  bool OpenTraceMarker();
};
}  // namespace gestures
#endif  // GESTURES_TRACE_MARKER_H__
