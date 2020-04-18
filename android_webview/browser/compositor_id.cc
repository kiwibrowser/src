// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/compositor_id.h"
#include "content/public/common/child_process_host.h"

namespace android_webview {

CompositorID::CompositorID()
    : process_id(content::ChildProcessHost::kInvalidUniqueID),
      routing_id(content::ChildProcessHost::kInvalidUniqueID) {}

CompositorID::CompositorID(int process_id, int routing_id)
    : process_id(process_id), routing_id(routing_id) {}

CompositorID::CompositorID(const CompositorID& other) = default;

CompositorID& CompositorID::operator=(const CompositorID& other) {
  process_id = other.process_id;
  routing_id = other.routing_id;
  return *this;
}

bool CompositorID::Equals(const CompositorID& other) const {
  return process_id == other.process_id && routing_id == other.routing_id;
}

bool CompositorIDComparator::operator()(const CompositorID& a,
                                        const CompositorID& b) const {
  if (a.process_id == b.process_id) {
    return a.routing_id < b.routing_id;
  }
  return a.process_id < b.process_id;
}

}  // namespace android_webview
