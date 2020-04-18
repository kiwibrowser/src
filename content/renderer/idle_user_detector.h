// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_IDLE_USER_DETECTOR_H_
#define CONTENT_RENDERER_IDLE_USER_DETECTOR_H_

#include "base/macros.h"

namespace content {

// Class which observes user input events and postpones
// idle notifications if the user is active.
class IdleUserDetector {
 public:
  IdleUserDetector();
  ~IdleUserDetector();

  void ActivityDetected();

 private:

  DISALLOW_COPY_AND_ASSIGN(IdleUserDetector);
};

}  // namespace content

#endif  // CONTENT_RENDERER_IDLE_USER_DETECTOR_H_
