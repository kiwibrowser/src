// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/device_media_to_mojo_adapter.h"

#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "media/capture/video/video_capture_buffer_pool_impl.h"
#include "media/capture/video/video_capture_buffer_tracker_factory_impl.h"
#include "media/capture/video/video_capture_jpeg_decoder.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "services/video_capture/receiver_mojo_to_media_adapter.h"

namespace video_capture {

DeviceMediaToMojoAdapter::DeviceMediaToMojoAdapter(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    std::unique_ptr<media::VideoCaptureDevice> device,
    const media::VideoCaptureJpegDecoderFactoryCB&
        jpeg_decoder_factory_callback)
    : service_ref_(std::move(service_ref)),
      device_(std::move(device)),
      jpeg_decoder_factory_callback_(jpeg_decoder_factory_callback),
      device_started_(false),
      weak_factory_(this) {}

DeviceMediaToMojoAdapter::~DeviceMediaToMojoAdapter() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (device_started_)
    device_->StopAndDeAllocate();
}

void DeviceMediaToMojoAdapter::Start(
    const media::VideoCaptureParams& requested_settings,
    mojom::ReceiverPtr receiver) {
  DCHECK(thread_checker_.CalledOnValidThread());
  receiver.set_connection_error_handler(
      base::Bind(&DeviceMediaToMojoAdapter::OnClientConnectionErrorOrClose,
                 weak_factory_.GetWeakPtr()));

  auto receiver_adapter =
      std::make_unique<ReceiverMojoToMediaAdapter>(std::move(receiver));
  auto media_receiver = std::make_unique<ReceiverOnTaskRunner>(
      std::move(receiver_adapter), base::ThreadTaskRunnerHandle::Get());

  // Create a dedicated buffer pool for the device usage session.
  auto buffer_tracker_factory =
      std::make_unique<media::VideoCaptureBufferTrackerFactoryImpl>();
  scoped_refptr<media::VideoCaptureBufferPool> buffer_pool(
      new media::VideoCaptureBufferPoolImpl(std::move(buffer_tracker_factory),
                                            max_buffer_pool_buffer_count()));

  auto device_client = std::make_unique<media::VideoCaptureDeviceClient>(
      std::move(media_receiver), buffer_pool, jpeg_decoder_factory_callback_);

  device_->AllocateAndStart(requested_settings, std::move(device_client));
  device_started_ = true;
}

void DeviceMediaToMojoAdapter::OnReceiverReportingUtilization(
    int32_t frame_feedback_id,
    double utilization) {
  DCHECK(thread_checker_.CalledOnValidThread());
  device_->OnUtilizationReport(frame_feedback_id, utilization);
}

void DeviceMediaToMojoAdapter::RequestRefreshFrame() {
  if (!device_started_)
    return;
  device_->RequestRefreshFrame();
}

void DeviceMediaToMojoAdapter::MaybeSuspend() {
  if (!device_started_)
    return;
  device_->MaybeSuspend();
}

void DeviceMediaToMojoAdapter::Resume() {
  if (!device_started_)
    return;
  device_->Resume();
}

void DeviceMediaToMojoAdapter::GetPhotoState(GetPhotoStateCallback callback) {
  media::VideoCaptureDevice::GetPhotoStateCallback scoped_callback =
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          media::BindToCurrentLoop(std::move(callback)), nullptr);
  device_->GetPhotoState(std::move(scoped_callback));
}

void DeviceMediaToMojoAdapter::SetPhotoOptions(
    media::mojom::PhotoSettingsPtr settings,
    SetPhotoOptionsCallback callback) {
  media::mojom::ImageCapture::SetOptionsCallback scoped_callback =
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          media::BindToCurrentLoop(std::move(callback)), false);
  device_->SetPhotoOptions(std::move(settings), std::move(scoped_callback));
}

void DeviceMediaToMojoAdapter::TakePhoto(TakePhotoCallback callback) {
  media::mojom::ImageCapture::TakePhotoCallback scoped_callback =
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          media::BindToCurrentLoop(std::move(callback)), nullptr);
  device_->TakePhoto(std::move(scoped_callback));
}

void DeviceMediaToMojoAdapter::Stop() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (device_started_ == false)
    return;
  device_started_ = false;
  weak_factory_.InvalidateWeakPtrs();
  device_->StopAndDeAllocate();
}

void DeviceMediaToMojoAdapter::OnClientConnectionErrorOrClose() {
  DCHECK(thread_checker_.CalledOnValidThread());
  Stop();
}

// static
int DeviceMediaToMojoAdapter::max_buffer_pool_buffer_count() {
  // The maximum number of video frame buffers in-flight at any one time.
  // If all buffers are still in use by consumers when new frames are produced
  // those frames get dropped.
  static const int kMaxBufferCount = 3;

  return kMaxBufferCount;
}

}  // namespace video_capture
