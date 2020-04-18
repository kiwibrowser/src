// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/service_launched_video_capture_device.h"

namespace content {

ServiceLaunchedVideoCaptureDevice::ServiceLaunchedVideoCaptureDevice(
    video_capture::mojom::DevicePtr device,
    base::OnceClosure connection_lost_cb)
    : device_(std::move(device)),
      connection_lost_cb_(std::move(connection_lost_cb)) {
  // Unretained |this| is safe, because |this| owns |device_|.
  device_.set_connection_error_handler(base::BindOnce(
      &ServiceLaunchedVideoCaptureDevice::OnLostConnectionToDevice,
      base::Unretained(this)));
}

ServiceLaunchedVideoCaptureDevice::~ServiceLaunchedVideoCaptureDevice() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
}

void ServiceLaunchedVideoCaptureDevice::GetPhotoState(
    media::VideoCaptureDevice::GetPhotoStateCallback callback) const {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  device_->GetPhotoState(base::BindOnce(
      &ServiceLaunchedVideoCaptureDevice::OnGetPhotoStateResponse,
      base::Unretained(this), std::move(callback)));
}

void ServiceLaunchedVideoCaptureDevice::SetPhotoOptions(
    media::mojom::PhotoSettingsPtr settings,
    media::VideoCaptureDevice::SetPhotoOptionsCallback callback) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  device_->SetPhotoOptions(
      std::move(settings),
      base::BindOnce(
          &ServiceLaunchedVideoCaptureDevice::OnSetPhotoOptionsResponse,
          base::Unretained(this), std::move(callback)));
}

void ServiceLaunchedVideoCaptureDevice::TakePhoto(
    media::VideoCaptureDevice::TakePhotoCallback callback) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  device_->TakePhoto(
      base::BindOnce(&ServiceLaunchedVideoCaptureDevice::OnTakePhotoResponse,
                     base::Unretained(this), std::move(callback)));
}

void ServiceLaunchedVideoCaptureDevice::MaybeSuspendDevice() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  device_->MaybeSuspend();
}

void ServiceLaunchedVideoCaptureDevice::ResumeDevice() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  device_->Resume();
}

void ServiceLaunchedVideoCaptureDevice::RequestRefreshFrame() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  device_->RequestRefreshFrame();
}

void ServiceLaunchedVideoCaptureDevice::SetDesktopCaptureWindowIdAsync(
    gfx::NativeViewId window_id,
    base::OnceClosure done_cb) {
  // This method should only be called for desktop capture devices.
  // The video_capture Mojo service does not support desktop capture devices
  // (yet) and should not be used for it.
  NOTREACHED();
}

void ServiceLaunchedVideoCaptureDevice::OnUtilizationReport(
    int frame_feedback_id,
    double utilization) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  device_->OnReceiverReportingUtilization(frame_feedback_id, utilization);
}

void ServiceLaunchedVideoCaptureDevice::OnLostConnectionToDevice() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  base::ResetAndReturn(&connection_lost_cb_).Run();
}

void ServiceLaunchedVideoCaptureDevice::OnGetPhotoStateResponse(
    media::VideoCaptureDevice::GetPhotoStateCallback callback,
    media::mojom::PhotoStatePtr capabilities) const {
  if (!capabilities)
    return;
  std::move(callback).Run(std::move(capabilities));
}

void ServiceLaunchedVideoCaptureDevice::OnSetPhotoOptionsResponse(
    media::VideoCaptureDevice::SetPhotoOptionsCallback callback,
    bool success) {
  if (!success)
    return;
  std::move(callback).Run(true);
}

void ServiceLaunchedVideoCaptureDevice::OnTakePhotoResponse(
    media::VideoCaptureDevice::TakePhotoCallback callback,
    media::mojom::BlobPtr blob) {
  if (!blob)
    return;
  std::move(callback).Run(std::move(blob));
}

}  // namespace content
