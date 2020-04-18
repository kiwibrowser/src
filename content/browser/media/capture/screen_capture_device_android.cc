// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/screen_capture_device_android.h"

#include "base/logging.h"
#include "media/capture/content/android/screen_capture_machine_android.h"

namespace content {

ScreenCaptureDeviceAndroid::ScreenCaptureDeviceAndroid()
    : core_(std::make_unique<media::ScreenCaptureMachineAndroid>()) {}

ScreenCaptureDeviceAndroid::~ScreenCaptureDeviceAndroid() {
  DVLOG(2) << "ScreenCaptureDeviceAndroid@" << this << " destroying.";
}

void ScreenCaptureDeviceAndroid::AllocateAndStart(
    const media::VideoCaptureParams& params,
    std::unique_ptr<Client> client) {
  DVLOG(1) << "Allocating " << params.requested_format.frame_size.ToString();
  core_.AllocateAndStart(params, std::move(client));
}

void ScreenCaptureDeviceAndroid::StopAndDeAllocate() {
  core_.StopAndDeAllocate();
}

void ScreenCaptureDeviceAndroid::RequestRefreshFrame() {
  core_.RequestRefreshFrame();
}

void ScreenCaptureDeviceAndroid::OnUtilizationReport(int frame_feedback_id,
                                                     double utilization) {
  core_.OnConsumerReportingUtilization(frame_feedback_id, utilization);
}

}  // namespace content
