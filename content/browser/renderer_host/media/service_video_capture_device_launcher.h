// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_VIDEO_CAPTURE_DEVICE_LAUNCHER_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_VIDEO_CAPTURE_DEVICE_LAUNCHER_H_

#include "content/browser/renderer_host/media/video_capture_factory_delegate.h"
#include "content/browser/renderer_host/media/video_capture_provider.h"
#include "content/public/browser/video_capture_device_launcher.h"
#include "content/public/common/media_stream_request.h"
#include "services/video_capture/public/mojom/device_factory.mojom.h"

namespace content {

// Implementation of VideoCaptureDeviceLauncher that uses the "video_capture"
// service.
class CONTENT_EXPORT ServiceVideoCaptureDeviceLauncher
    : public VideoCaptureDeviceLauncher {
 public:
  // Receives an instance via output parameter |factory|.
  using ConnectToDeviceFactoryCB = base::RepeatingCallback<void(
      std::unique_ptr<VideoCaptureFactoryDelegate>* factory)>;

  explicit ServiceVideoCaptureDeviceLauncher(
      ConnectToDeviceFactoryCB connect_to_device_factory_cb);
  ~ServiceVideoCaptureDeviceLauncher() override;

  // VideoCaptureDeviceLauncher implementation.
  void LaunchDeviceAsync(const std::string& device_id,
                         MediaStreamType stream_type,
                         const media::VideoCaptureParams& params,
                         base::WeakPtr<media::VideoFrameReceiver> receiver,
                         base::OnceClosure connection_lost_cb,
                         Callbacks* callbacks,
                         base::OnceClosure done_cb) override;
  void AbortLaunch() override;

  void OnUtilizationReport(int frame_feedback_id, double utilization);

 private:
  enum class State {
    READY_TO_LAUNCH,
    DEVICE_START_IN_PROGRESS,
    DEVICE_START_ABORTING
  };

  void OnCreateDeviceCallback(
      const media::VideoCaptureParams& params,
      video_capture::mojom::DevicePtr device,
      base::WeakPtr<media::VideoFrameReceiver> receiver,
      base::OnceClosure connection_lost_cb,
      video_capture::mojom::DeviceAccessResultCode result_code);

  void OnConnectionLostWhileWaitingForCallback();

  ConnectToDeviceFactoryCB connect_to_device_factory_cb_;
  std::unique_ptr<VideoCaptureFactoryDelegate> device_factory_;
  State state_;
  base::SequenceChecker sequence_checker_;
  base::OnceClosure done_cb_;
  Callbacks* callbacks_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_VIDEO_CAPTURE_DEVICE_LAUNCHER_H_
