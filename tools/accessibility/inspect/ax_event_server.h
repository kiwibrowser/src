// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AX_EVENT_SERVER_H_
#define AX_EVENT_SERVER_H_

#include <string>

#include "content/browser/accessibility/accessibility_event_recorder.h"

namespace content {

class AXEventServer {
 public:
  explicit AXEventServer(int pid);

  ~AXEventServer();

 private:
  std::unique_ptr<AccessibilityEventRecorder> recorder_;
};

}  // namespace content

#endif  // AX_EVENT_SERVER_H_
