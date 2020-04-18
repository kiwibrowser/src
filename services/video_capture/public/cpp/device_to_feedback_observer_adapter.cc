// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/public/cpp/device_to_feedback_observer_adapter.h"

namespace video_capture {

DeviceToFeedbackObserverAdapter::DeviceToFeedbackObserverAdapter(
    mojom::DevicePtr device)
    : device_(std::move(device)) {}

DeviceToFeedbackObserverAdapter::~DeviceToFeedbackObserverAdapter() = default;

void DeviceToFeedbackObserverAdapter::OnUtilizationReport(int frame_feedback_id,
                                                          double utilization) {
  device_->OnReceiverReportingUtilization(frame_feedback_id, utilization);
}

}  // namespace video_capture
