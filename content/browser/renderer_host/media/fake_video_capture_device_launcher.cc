// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "content/browser/renderer_host/media/fake_video_capture_device_launcher.h"
#include "media/capture/video/video_capture_buffer_pool_impl.h"
#include "media/capture/video/video_capture_buffer_tracker_factory_impl.h"
#include "media/capture/video/video_capture_device_client.h"
#include "media/capture/video/video_capture_jpeg_decoder.h"
#include "media/capture/video/video_frame_receiver_on_task_runner.h"

namespace {

static const int kMaxBufferCount = 3;

class FakeLaunchedVideoCaptureDevice
    : public content::LaunchedVideoCaptureDevice {
 public:
  FakeLaunchedVideoCaptureDevice(
      std::unique_ptr<media::VideoCaptureDevice> device)
      : device_(std::move(device)) {}

  void GetPhotoState(media::VideoCaptureDevice::GetPhotoStateCallback callback)
      const override {
    device_->GetPhotoState(std::move(callback));
  }
  void SetPhotoOptions(
      media::mojom::PhotoSettingsPtr settings,
      media::VideoCaptureDevice::SetPhotoOptionsCallback callback) override {
    device_->SetPhotoOptions(std::move(settings), std::move(callback));
  }
  void TakePhoto(
      media::VideoCaptureDevice::TakePhotoCallback callback) override {
    device_->TakePhoto(std::move(callback));
  }
  void MaybeSuspendDevice() override { device_->MaybeSuspend(); }
  void ResumeDevice() override { device_->Resume(); }
  void RequestRefreshFrame() override { device_->RequestRefreshFrame(); }
  void SetDesktopCaptureWindowIdAsync(gfx::NativeViewId window_id,
                                      base::OnceClosure done_cb) override {
    // Do nothing.
  }
  void OnUtilizationReport(int frame_feedback_id, double utilization) override {
    device_->OnUtilizationReport(frame_feedback_id, utilization);
  }

 private:
  std::unique_ptr<media::VideoCaptureDevice> device_;
};

}  // anonymous namespace

namespace content {

FakeVideoCaptureDeviceLauncher::FakeVideoCaptureDeviceLauncher(
    media::VideoCaptureSystem* system)
    : system_(system) {
  DCHECK(system_);
}

FakeVideoCaptureDeviceLauncher::~FakeVideoCaptureDeviceLauncher() = default;

void FakeVideoCaptureDeviceLauncher::LaunchDeviceAsync(
    const std::string& device_id,
    content::MediaStreamType stream_type,
    const media::VideoCaptureParams& params,
    base::WeakPtr<media::VideoFrameReceiver> receiver,
    base::OnceClosure connection_lost_cb,
    Callbacks* callbacks,
    base::OnceClosure done_cb) {
  auto device = system_->CreateDevice(device_id);
  auto empty_jpeg_decoder_factory_cb = base::BindRepeating(
      []() { return std::unique_ptr<media::VideoCaptureJpegDecoder>(); });
  scoped_refptr<media::VideoCaptureBufferPool> buffer_pool(
      new media::VideoCaptureBufferPoolImpl(
          std::make_unique<media::VideoCaptureBufferTrackerFactoryImpl>(),
          kMaxBufferCount));
  auto device_client = std::make_unique<media::VideoCaptureDeviceClient>(
      std::make_unique<media::VideoFrameReceiverOnTaskRunner>(
          receiver, base::ThreadTaskRunnerHandle::Get()),
      std::move(buffer_pool), std::move(empty_jpeg_decoder_factory_cb));
  device->AllocateAndStart(params, std::move(device_client));
  auto launched_device =
      std::make_unique<FakeLaunchedVideoCaptureDevice>(std::move(device));
  callbacks->OnDeviceLaunched(std::move(launched_device));
}

void FakeVideoCaptureDeviceLauncher::AbortLaunch() {
  // Do nothing.
}

}  // namespace content
