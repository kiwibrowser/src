// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mirroring/service/fake_video_capture_host.h"

#include "media/base/video_frame.h"
#include "mojo/public/cpp/system/buffer.h"

namespace mirroring {

FakeVideoCaptureHost::FakeVideoCaptureHost(
    media::mojom::VideoCaptureHostRequest request)
    : binding_(this, std::move(request)) {}
FakeVideoCaptureHost::~FakeVideoCaptureHost() {}

void FakeVideoCaptureHost::Start(
    int32_t device_id,
    int32_t session_id,
    const media::VideoCaptureParams& params,
    media::mojom::VideoCaptureObserverPtr observer) {
  ASSERT_TRUE(observer);
  observer_ = std::move(observer);
  observer_->OnStateChanged(media::mojom::VideoCaptureState::STARTED);
}

void FakeVideoCaptureHost::Stop(int32_t device_id) {
  if (!observer_)
    return;

  observer_->OnStateChanged(media::mojom::VideoCaptureState::ENDED);
  observer_.reset();
  OnStopped();
}

void FakeVideoCaptureHost::SendOneFrame(const gfx::Size& size,
                                        base::TimeTicks capture_time) {
  if (!observer_)
    return;

  mojo::ScopedSharedBufferHandle buffer =
      mojo::SharedBufferHandle::Create(5000);
  memset(buffer->Map(5000).get(), 125, 5000);
  media::mojom::VideoBufferHandlePtr buffer_handle =
      media::mojom::VideoBufferHandle::New();
  buffer_handle->set_shared_buffer_handle(std::move(buffer));
  observer_->OnNewBuffer(0, std::move(buffer_handle));
  media::VideoFrameMetadata metadata;
  metadata.SetDouble(media::VideoFrameMetadata::FRAME_RATE, 30);
  metadata.SetTimeTicks(media::VideoFrameMetadata::REFERENCE_TIME,
                        capture_time);
  observer_->OnBufferReady(
      0, media::mojom::VideoFrameInfo::New(
             base::TimeDelta(), metadata.GetInternalValues().Clone(),
             media::PIXEL_FORMAT_I420, size, gfx::Rect(size)));
}

}  // namespace mirroring
