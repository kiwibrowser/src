// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/chromeos/camera_device_context.h"

namespace media {

CameraDeviceContext::CameraDeviceContext(
    std::unique_ptr<VideoCaptureDevice::Client> client)
    : state_(State::kStopped),
      sensor_orientation_(0),
      screen_rotation_(0),
      client_(std::move(client)) {
  DCHECK(client_);
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

CameraDeviceContext::~CameraDeviceContext() = default;

void CameraDeviceContext::SetState(State state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  state_ = state;
  if (state_ == State::kCapturing) {
    client_->OnStarted();
  }
}

CameraDeviceContext::State CameraDeviceContext::GetState() {
  return state_;
}

void CameraDeviceContext::SetErrorState(const base::Location& from_here,
                                        const std::string& reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  state_ = State::kError;
  LOG(ERROR) << reason;
  client_->OnError(from_here, reason);
}

void CameraDeviceContext::LogToClient(std::string message) {
  client_->OnLog(message);
}

void CameraDeviceContext::SubmitCapturedData(
    gfx::GpuMemoryBuffer* buffer,
    const VideoCaptureFormat& frame_format,
    base::TimeTicks reference_time,
    base::TimeDelta timestamp) {
  client_->OnIncomingCapturedGfxBuffer(buffer, frame_format,
                                       GetCameraFrameOrientation(),
                                       reference_time, timestamp);
}

void CameraDeviceContext::SetSensorOrientation(int sensor_orientation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sensor_orientation >= 0 && sensor_orientation < 360 &&
         sensor_orientation % 90 == 0);
  sensor_orientation_ = sensor_orientation;
}

void CameraDeviceContext::SetScreenRotation(int screen_rotation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(screen_rotation >= 0 && screen_rotation < 360 &&
         screen_rotation % 90 == 0);
  screen_rotation_ = screen_rotation;
}

int CameraDeviceContext::GetCameraFrameOrientation() {
  return (sensor_orientation_ + screen_rotation_) % 360;
}

}  // namespace media
