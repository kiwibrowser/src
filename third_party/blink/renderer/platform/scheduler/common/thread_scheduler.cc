// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"

#include "third_party/blink/public/platform/platform.h"

namespace blink {

ThreadScheduler* ThreadScheduler::Current() {
  return Platform::Current()->CurrentThread()->Scheduler();
}

}  // namespace blink
