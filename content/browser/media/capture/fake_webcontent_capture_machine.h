// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_FAKE_WEBCONTENT_CAPTURE_MACHINE_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_FAKE_WEBCONTENT_CAPTURE_MACHINE_H_

#include "base/callback_helpers.h"
#include "content/common/content_export.h"
#include "media/capture/content/screen_capture_device_core.h"
#include "media/capture/content/thread_safe_capture_oracle.h"
#include "media/capture/video_capture_types.h"

namespace content {

// An implementation of VideoCaptureDevice that fakes a desktop capturer.
class CONTENT_EXPORT FakeWebContentCaptureMachine
    : public media::VideoCaptureMachine {
 public:
  FakeWebContentCaptureMachine(bool enable_auto_throttling);
  ~FakeWebContentCaptureMachine() override;

  // VideoCaptureMachine overrides.
  void Start(const scoped_refptr<media::ThreadSafeCaptureOracle>& oracle_proxy,
             const media::VideoCaptureParams& params,
             const base::Callback<void(bool)> callback) override;
  void Suspend() override;
  void Resume() override;
  void Stop(const base::Closure& callback) override;
  bool IsAutoThrottlingEnabled() const override;
  void MaybeCaptureForRefresh() override;

 private:
  bool enable_auto_throttling_;

  DISALLOW_COPY_AND_ASSIGN(FakeWebContentCaptureMachine);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_DESKTOP_FAKE_CAPTURE_DEVICE_H_