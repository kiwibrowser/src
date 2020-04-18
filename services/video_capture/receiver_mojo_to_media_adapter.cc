// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/receiver_mojo_to_media_adapter.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/video_capture/scoped_access_permission_media_to_mojo_adapter.h"

namespace video_capture {

ReceiverOnTaskRunner::ReceiverOnTaskRunner(
    std::unique_ptr<media::VideoFrameReceiver> receiver,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : receiver_(std::move(receiver)), task_runner_(std::move(task_runner)) {}

ReceiverOnTaskRunner::~ReceiverOnTaskRunner() {
  task_runner_->DeleteSoon(FROM_HERE, receiver_.release());
}

void ReceiverOnTaskRunner::OnNewBuffer(
    int buffer_id,
    media::mojom::VideoBufferHandlePtr buffer_handle) {
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&media::VideoFrameReceiver::OnNewBuffer,
                                base::Unretained(receiver_.get()), buffer_id,
                                base::Passed(&buffer_handle)));
}

void ReceiverOnTaskRunner::OnFrameReadyInBuffer(
    int buffer_id,
    int frame_feedback_id,
    std::unique_ptr<
        media::VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>
        buffer_read_permission,
    media::mojom::VideoFrameInfoPtr frame_info) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&media::VideoFrameReceiver::OnFrameReadyInBuffer,
                 base::Unretained(receiver_.get()), buffer_id,
                 frame_feedback_id, base::Passed(&buffer_read_permission),
                 base::Passed(&frame_info)));
}

void ReceiverOnTaskRunner::OnBufferRetired(int buffer_id) {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&media::VideoFrameReceiver::OnBufferRetired,
                            base::Unretained(receiver_.get()), buffer_id));
}

void ReceiverOnTaskRunner::OnError() {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&media::VideoFrameReceiver::OnError,
                                    base::Unretained(receiver_.get())));
}

void ReceiverOnTaskRunner::OnLog(const std::string& message) {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&media::VideoFrameReceiver::OnLog,
                            base::Unretained(receiver_.get()), message));
}

void ReceiverOnTaskRunner::OnStarted() {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&media::VideoFrameReceiver::OnStarted,
                                    base::Unretained(receiver_.get())));
}

void ReceiverOnTaskRunner::OnStartedUsingGpuDecode() {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&media::VideoFrameReceiver::OnStartedUsingGpuDecode,
                            base::Unretained(receiver_.get())));
}

ReceiverMojoToMediaAdapter::ReceiverMojoToMediaAdapter(
    mojom::ReceiverPtr receiver)
    : receiver_(std::move(receiver)) {}

ReceiverMojoToMediaAdapter::~ReceiverMojoToMediaAdapter() = default;

void ReceiverMojoToMediaAdapter::OnNewBuffer(
    int buffer_id,
    media::mojom::VideoBufferHandlePtr buffer_handle) {
  receiver_->OnNewBuffer(buffer_id, std::move(buffer_handle));
}

void ReceiverMojoToMediaAdapter::OnFrameReadyInBuffer(
    int buffer_id,
    int frame_feedback_id,
    std::unique_ptr<
        media::VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>
        access_permission,
    media::mojom::VideoFrameInfoPtr frame_info) {
  mojom::ScopedAccessPermissionPtr access_permission_proxy;
  mojo::MakeStrongBinding<mojom::ScopedAccessPermission>(
      std::make_unique<ScopedAccessPermissionMediaToMojoAdapter>(
          std::move(access_permission)),
      mojo::MakeRequest(&access_permission_proxy));
  receiver_->OnFrameReadyInBuffer(buffer_id, frame_feedback_id,
                                  std::move(access_permission_proxy),
                                  std::move(frame_info));
}

void ReceiverMojoToMediaAdapter::OnBufferRetired(int buffer_id) {
  receiver_->OnBufferRetired(buffer_id);
}

void ReceiverMojoToMediaAdapter::OnError() {
  receiver_->OnError();
}

void ReceiverMojoToMediaAdapter::OnLog(const std::string& message) {
  receiver_->OnLog(message);
}

void ReceiverMojoToMediaAdapter::OnStarted() {
  receiver_->OnStarted();
}

void ReceiverMojoToMediaAdapter::OnStartedUsingGpuDecode() {
  receiver_->OnStartedUsingGpuDecode();
}

}  // namespace video_capture
