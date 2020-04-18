// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_FACTORY_DELEGATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_FACTORY_DELEGATE_H_

#include "content/common/content_export.h"
#include "services/video_capture/public/mojom/device_factory.mojom.h"

namespace content {

// Forwards calls to a given video_capture::mojom::DeviceFactoryPtr and invokes
// a given OnceClosure on destruction.
// Before calling CreatDevice(), clients must verify that is_bound() returns
// true. When is_bound() returns false, calling CreateDevice() is illegal.
// Note: This can happen when the ServiceVideoCaptureProvider owning
// |device_factory_| loses connection to the service process and resets
// |device_factory_|.
class CONTENT_EXPORT VideoCaptureFactoryDelegate {
 public:
  VideoCaptureFactoryDelegate(
      video_capture::mojom::DeviceFactoryPtr* device_factory,
      base::OnceClosure destruction_cb);

  ~VideoCaptureFactoryDelegate();

  bool is_bound() { return device_factory_->is_bound(); }

  void CreateDevice(
      const std::string& device_id,
      ::video_capture::mojom::DeviceRequest device_request,
      video_capture::mojom::DeviceFactory::CreateDeviceCallback callback);

 private:
  video_capture::mojom::DeviceFactoryPtr* const device_factory_;
  base::OnceClosure destruction_cb_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_FACTORY_DELEGATE_H_
