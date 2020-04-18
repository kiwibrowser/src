// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_SCREEN_CAPTURE_DEVICE_ANDROID_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_SCREEN_CAPTURE_DEVICE_ANDROID_H_

#include <memory>

#include "content/common/content_export.h"
#include "media/capture/content/screen_capture_device_core.h"
#include "media/capture/video/video_capture_device.h"

namespace content {

// ScreenCaptureDeviceAndroid is a forwarder to media::ScreenCaptureDeviceCore
// while keeping the Power Saving from kicking in between AllocateAndStart() and
// StopAndDeAllocate().
class CONTENT_EXPORT ScreenCaptureDeviceAndroid
    : public media::VideoCaptureDevice {
 public:
  ScreenCaptureDeviceAndroid();
  ~ScreenCaptureDeviceAndroid() override;

  // VideoCaptureDevice implementation.
  void AllocateAndStart(const media::VideoCaptureParams& params,
                        std::unique_ptr<Client> client) override;
  void StopAndDeAllocate() override;
  void RequestRefreshFrame() override;
  void OnUtilizationReport(int frame_feedback_id, double utilization) override;

 private:
  media::ScreenCaptureDeviceCore core_;

  DISALLOW_COPY_AND_ASSIGN(ScreenCaptureDeviceAndroid);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_SCREEN_CAPTURE_DEVICE_ANDROID_H_
