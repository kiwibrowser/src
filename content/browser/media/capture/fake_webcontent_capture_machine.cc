// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/fake_webcontent_capture_machine.h"

#include "base/logging.h"

namespace content {

FakeWebContentCaptureMachine::FakeWebContentCaptureMachine(
    bool enable_auto_throttling)
    : enable_auto_throttling_(enable_auto_throttling) {
  DVLOG(2) << "FakeWebContentCaptureMachine";
}

FakeWebContentCaptureMachine::~FakeWebContentCaptureMachine() {
  DVLOG(2) << "FakeWebContentCaptureMachine@" << this << " destroying.";
}

void FakeWebContentCaptureMachine::Start(
    const scoped_refptr<media::ThreadSafeCaptureOracle>& oracle_proxy,
    const media::VideoCaptureParams& params,
    const base::Callback<void(bool)> callback) {
  callback.Run(true);
}
void FakeWebContentCaptureMachine::Suspend() {}
void FakeWebContentCaptureMachine::Resume() {}
void FakeWebContentCaptureMachine::Stop(const base::Closure& callback) {}
bool FakeWebContentCaptureMachine::IsAutoThrottlingEnabled() const {
  return enable_auto_throttling_;
}
void FakeWebContentCaptureMachine::MaybeCaptureForRefresh() {}

}  // namespace content