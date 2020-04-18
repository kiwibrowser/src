// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/video_frame_receiver_on_task_runner.h"

#include "base/single_thread_task_runner.h"

namespace media {

VideoFrameReceiverOnTaskRunner::VideoFrameReceiverOnTaskRunner(
    const base::WeakPtr<VideoFrameReceiver>& receiver,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : receiver_(receiver), task_runner_(std::move(task_runner)) {}

VideoFrameReceiverOnTaskRunner::~VideoFrameReceiverOnTaskRunner() = default;

void VideoFrameReceiverOnTaskRunner::OnNewBuffer(
    int buffer_id,
    media::mojom::VideoBufferHandlePtr buffer_handle) {
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VideoFrameReceiver::OnNewBuffer, receiver_, buffer_id,
                     base::Passed(std::move(buffer_handle))));
}

void VideoFrameReceiverOnTaskRunner::OnFrameReadyInBuffer(
    int buffer_id,
    int frame_feedback_id,
    std::unique_ptr<VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>
        buffer_read_permission,
    mojom::VideoFrameInfoPtr frame_info) {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&VideoFrameReceiver::OnFrameReadyInBuffer,
                                    receiver_, buffer_id, frame_feedback_id,
                                    base::Passed(&buffer_read_permission),
                                    base::Passed(&frame_info)));
}

void VideoFrameReceiverOnTaskRunner::OnBufferRetired(int buffer_id) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&VideoFrameReceiver::OnBufferRetired, receiver_, buffer_id));
}

void VideoFrameReceiverOnTaskRunner::OnError() {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&VideoFrameReceiver::OnError, receiver_));
}

void VideoFrameReceiverOnTaskRunner::OnLog(const std::string& message) {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&VideoFrameReceiver::OnLog, receiver_, message));
}

void VideoFrameReceiverOnTaskRunner::OnStarted() {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&VideoFrameReceiver::OnStarted, receiver_));
}

void VideoFrameReceiverOnTaskRunner::OnStartedUsingGpuDecode() {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&VideoFrameReceiver::OnStartedUsingGpuDecode, receiver_));
}

}  // namespace media
