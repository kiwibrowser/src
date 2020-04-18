// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/host/client_frame_sink_video_capturer.h"

namespace viz {

namespace {

// How long to wait before attempting to re-establish a lost connection.
constexpr base::TimeDelta kReEstablishConnectionDelay =
    base::TimeDelta::FromMilliseconds(100);

}  // namespace

ClientFrameSinkVideoCapturer::ClientFrameSinkVideoCapturer(
    EstablishConnectionCallback callback)
    : establish_connection_callback_(callback),
      consumer_binding_(this),
      weak_factory_(this) {
  EstablishConnection();
}

ClientFrameSinkVideoCapturer::~ClientFrameSinkVideoCapturer() = default;

void ClientFrameSinkVideoCapturer::SetFormat(media::VideoPixelFormat format,
                                             media::ColorSpace color_space) {
  format_.emplace(format, color_space);
  capturer_->SetFormat(format, color_space);
}

void ClientFrameSinkVideoCapturer::SetMinCapturePeriod(
    base::TimeDelta min_capture_period) {
  min_capture_period_ = min_capture_period;
  capturer_->SetMinCapturePeriod(min_capture_period);
}

void ClientFrameSinkVideoCapturer::SetMinSizeChangePeriod(
    base::TimeDelta min_period) {
  min_size_change_period_ = min_period;
  capturer_->SetMinSizeChangePeriod(min_period);
}

void ClientFrameSinkVideoCapturer::SetResolutionConstraints(
    const gfx::Size& min_size,
    const gfx::Size& max_size,
    bool use_fixed_aspect_ratio) {
  resolution_constraints_.emplace(min_size, max_size, use_fixed_aspect_ratio);
  capturer_->SetResolutionConstraints(min_size, max_size,
                                      use_fixed_aspect_ratio);
}

void ClientFrameSinkVideoCapturer::SetAutoThrottlingEnabled(bool enabled) {
  auto_throttling_enabled_ = enabled;
  capturer_->SetAutoThrottlingEnabled(enabled);
}

void ClientFrameSinkVideoCapturer::ChangeTarget(
    const FrameSinkId& frame_sink_id) {
  target_ = frame_sink_id;
  capturer_->ChangeTarget(frame_sink_id);
}

void ClientFrameSinkVideoCapturer::Start(
    mojom::FrameSinkVideoConsumer* consumer) {
  DCHECK(consumer);
  is_started_ = true;
  consumer_ = consumer;
  StartInternal();
}

void ClientFrameSinkVideoCapturer::Stop() {
  is_started_ = false;
  capturer_->Stop();
}

void ClientFrameSinkVideoCapturer::StopAndResetConsumer() {
  Stop();
  consumer_ = nullptr;
  consumer_binding_.Close();
}

void ClientFrameSinkVideoCapturer::RequestRefreshFrame() {
  capturer_->RequestRefreshFrame();
}

ClientFrameSinkVideoCapturer::Format::Format(
    media::VideoPixelFormat pixel_format,
    media::ColorSpace color_space)
    : pixel_format(pixel_format), color_space(color_space) {}

ClientFrameSinkVideoCapturer::ResolutionConstraints::ResolutionConstraints(
    const gfx::Size& min_size,
    const gfx::Size& max_size,
    bool use_fixed_aspect_ratio)
    : min_size(min_size),
      max_size(max_size),
      use_fixed_aspect_ratio(use_fixed_aspect_ratio) {}

void ClientFrameSinkVideoCapturer::OnFrameCaptured(
    mojo::ScopedSharedBufferHandle buffer,
    uint32_t buffer_size,
    media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& update_rect,
    const gfx::Rect& content_rect,
    mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) {
  consumer_->OnFrameCaptured(std::move(buffer), buffer_size, std::move(info),
                             update_rect, content_rect, std::move(callbacks));
}

void ClientFrameSinkVideoCapturer::OnTargetLost(
    const FrameSinkId& frame_sink_id) {
  consumer_->OnTargetLost(frame_sink_id);
}

void ClientFrameSinkVideoCapturer::OnStopped() {
  consumer_->OnStopped();
}

void ClientFrameSinkVideoCapturer::EstablishConnection() {
  establish_connection_callback_.Run(mojo::MakeRequest(&capturer_));
  capturer_.set_connection_error_handler(
      base::BindOnce(&ClientFrameSinkVideoCapturer::OnConnectionError,
                     base::Unretained(this)));
  if (format_)
    capturer_->SetFormat(format_->pixel_format, format_->color_space);
  if (min_capture_period_)
    capturer_->SetMinCapturePeriod(*min_capture_period_);
  if (min_size_change_period_)
    capturer_->SetMinSizeChangePeriod(*min_size_change_period_);
  if (resolution_constraints_) {
    capturer_->SetResolutionConstraints(
        resolution_constraints_->min_size, resolution_constraints_->max_size,
        resolution_constraints_->use_fixed_aspect_ratio);
  }
  if (auto_throttling_enabled_)
    capturer_->SetAutoThrottlingEnabled(*auto_throttling_enabled_);
  if (target_)
    capturer_->ChangeTarget(*target_);
  if (is_started_)
    StartInternal();
}

void ClientFrameSinkVideoCapturer::OnConnectionError() {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ClientFrameSinkVideoCapturer::EstablishConnection,
                     weak_factory_.GetWeakPtr()),
      kReEstablishConnectionDelay);
}

void ClientFrameSinkVideoCapturer::StartInternal() {
  if (consumer_binding_)
    consumer_binding_.Close();
  mojom::FrameSinkVideoConsumerPtr consumer;
  consumer_binding_.Bind(mojo::MakeRequest(&consumer));
  capturer_->Start(std::move(consumer));
}

}  // namespace viz
