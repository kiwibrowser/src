// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/video_capture_factory_delegate.h"

namespace content {

VideoCaptureFactoryDelegate::VideoCaptureFactoryDelegate(
    video_capture::mojom::DeviceFactoryPtr* device_factory,
    base::OnceClosure destruction_cb)
    : device_factory_(device_factory),
      destruction_cb_(std::move(destruction_cb)) {}

VideoCaptureFactoryDelegate::~VideoCaptureFactoryDelegate() {
  base::ResetAndReturn(&destruction_cb_).Run();
}

void VideoCaptureFactoryDelegate::CreateDevice(
    const std::string& device_id,
    ::video_capture::mojom::DeviceRequest device_request,
    video_capture::mojom::DeviceFactory::CreateDeviceCallback callback) {
  DCHECK(device_factory_->is_bound());
  (*device_factory_)
      ->CreateDevice(device_id, std::move(device_request), std::move(callback));
}

}  // namespace content
