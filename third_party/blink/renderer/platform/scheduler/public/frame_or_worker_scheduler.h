// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_FRAME_OR_WORKER_SCHEDULER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_FRAME_OR_WORKER_SCHEDULER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

// This is the base class of FrameScheduler and WorkerScheduler.
class FrameOrWorkerScheduler {
  USING_FAST_MALLOC(FrameOrWorkerScheduler);

 public:
  virtual ~FrameOrWorkerScheduler() = default;

  class ActiveConnectionHandle {
   public:
    ActiveConnectionHandle() = default;
    virtual ~ActiveConnectionHandle() = default;

   private:
    DISALLOW_COPY_AND_ASSIGN(ActiveConnectionHandle);
  };

  // Notifies scheduler that this execution context has established an active
  // real time connection (websocket, webrtc, etc). When connection is closed
  // this handle must be destroyed.
  virtual std::unique_ptr<ActiveConnectionHandle>
  OnActiveConnectionCreated() = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_FRAME_OR_WORKER_SCHEDULER_H_
