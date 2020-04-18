// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/trace_marker.h"

#include <fcntl.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gestures/include/eintr_wrapper.h"
#include "gestures/include/logging.h"

namespace gestures {

void TraceMarker::CreateTraceMarker() {
  if (!trace_marker_)
    trace_marker_ = new TraceMarker();
  trace_marker_count_++;
}

void TraceMarker::DeleteTraceMarker() {
  if (trace_marker_count_ == 1) {
    delete trace_marker_;
    trace_marker_ = NULL;
  }
  trace_marker_count_--;
  if (trace_marker_count_ < 0)
    trace_marker_count_ = 0;
}

void TraceMarker::StaticTraceWrite(const char* str) {
  if (TraceMarker::GetTraceMarker())
    TraceMarker::GetTraceMarker()->TraceWrite(str);
  else
    Err("No TraceMarker Object");
}

TraceMarker* TraceMarker::GetTraceMarker() {
  return trace_marker_;
}

void TraceMarker::TraceWrite(const char* str) {
  if (fd_ == -1) {
    Err("Trace_marker does not open");
    return;
  }
  ssize_t len = strlen(str);
  ssize_t get = write(fd_, str, len);
  if (get == -1)
    Err("Write failed");
  else if (get != len)
    Err("Message too long!");
}

TraceMarker::TraceMarker() : fd_(-1) {
  if (!OpenTraceMarker())
    Log("Cannot open trace_marker");
}

TraceMarker::~TraceMarker() {
  if (fd_ != -1)
    close(fd_);
}

TraceMarker* TraceMarker::trace_marker_ = NULL;
int TraceMarker::trace_marker_count_ = 0;

bool TraceMarker::FindDebugfs(const char** ret) const {
  FILE* mounts = setmntent("/proc/mounts", "r");
  if (mounts == NULL)
    return false;

  while (true) {
    struct mntent* entry = getmntent(mounts);
    if (entry == NULL) {
      fclose(mounts);
      return false;
    }
    if (strcmp("debugfs", entry->mnt_type) == 0) {
      *ret = entry->mnt_dir;
      break;
    }
  }
  fclose(mounts);
  return true;
}

bool TraceMarker::FindTraceMarker(char** ret) const {
  const char* debugfs = NULL;
  if (!FindDebugfs(&debugfs))
    return false;
  if (asprintf(ret, "%s/tracing/trace_marker", debugfs) == -1) {
    *ret = NULL;
    return false;
  }
  return true;
}

bool TraceMarker::OpenTraceMarker() {
  char* trace_marker_filename = NULL;
  if (!FindTraceMarker(&trace_marker_filename))
    return false;
  fd_ = HANDLE_EINTR(open(trace_marker_filename, O_WRONLY));
  free(trace_marker_filename);
  if (fd_ == -1)
    return false;
  return true;
}
}  // namespace gestures
