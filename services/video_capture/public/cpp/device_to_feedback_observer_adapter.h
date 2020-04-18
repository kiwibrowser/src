// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_PUBLIC_CPP_DEVICE_TO_FEEDBACK_OBSERVER_ADAPTER_H_
#define SERVICES_VIDEO_CAPTURE_PUBLIC_CPP_DEVICE_TO_FEEDBACK_OBSERVER_ADAPTER_H_

#include "media/capture/video/video_capture_device.h"
#include "services/video_capture/public/mojom/device.mojom.h"

namespace video_capture {

// Adapter that allows a video_capture::mojom::Device to be used in place of
// a media::VideoFrameConsumerFeedbackObserver
class DeviceToFeedbackObserverAdapter
    : public media::VideoFrameConsumerFeedbackObserver {
 public:
  DeviceToFeedbackObserverAdapter(mojom::DevicePtr device);
  ~DeviceToFeedbackObserverAdapter() override;

  // media::VideoFrameConsumerFeedbackObserver:
  void OnUtilizationReport(int frame_feedback_id, double utilization) override;

 private:
  mojom::DevicePtr device_;
};

}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_PUBLIC_CPP_DEVICE_TO_FEEDBACK_OBSERVER_ADAPTER_H_
