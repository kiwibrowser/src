// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_recorder/vea_encoder.h"

#include <string>

#include "base/containers/queue.h"
#include "base/metrics/histogram_macros.h"
#include "content/renderer/media/gpu/gpu_video_accelerator_factories_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/video_frame.h"
#include "third_party/libyuv/include/libyuv.h"
#include "ui/gfx/geometry/size.h"

using media::VideoFrame;
using video_track_recorder::kVEAEncoderMinResolutionWidth;
using video_track_recorder::kVEAEncoderMinResolutionHeight;

namespace content {

namespace {

// HW encoders expect a nonzero bitrate, so |kVEADefaultBitratePerPixel| is used
// to estimate bits per second for ~30 fps with ~1/16 compression rate.
const int kVEADefaultBitratePerPixel = 2;
// Number of output buffers used to copy the encoded data coming from HW
// encoders.
const int kVEAEncoderOutputBufferCount = 4;
// Force a keyframe in regular intervals.
const uint32_t kMaxKeyframeInterval = 100;

}  // anonymous namespace

VEAEncoder::VEAEncoder(
    const VideoTrackRecorder::OnEncodedVideoCB& on_encoded_video_callback,
    const VideoTrackRecorder::OnErrorCB& on_error_callback,
    int32_t bits_per_second,
    media::VideoCodecProfile codec,
    const gfx::Size& size,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : Encoder(on_encoded_video_callback,
              bits_per_second > 0 ? bits_per_second
                                  : size.GetArea() * kVEADefaultBitratePerPixel,
              std::move(task_runner),
              RenderThreadImpl::current()->GetGpuFactories()->GetTaskRunner()),
      gpu_factories_(RenderThreadImpl::current()->GetGpuFactories()),
      codec_(codec),
      error_notified_(false),
      num_frames_after_keyframe_(0),
      force_next_frame_to_be_keyframe_(false),
      on_error_callback_(on_error_callback) {
  DCHECK(gpu_factories_);
  DCHECK_GE(size.width(), kVEAEncoderMinResolutionWidth);
  DCHECK_GE(size.height(), kVEAEncoderMinResolutionHeight);

  encoding_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&VEAEncoder::ConfigureEncoderOnEncodingTaskRunner, this,
                     size));
}

VEAEncoder::~VEAEncoder() {
  base::WaitableEvent release_waiter(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  // base::Unretained is safe because the class will be alive until
  // |release_waiter| is signaled.
  // TODO(emircan): Consider refactoring media::VideoEncodeAccelerator to avoid
  // using naked pointers and using DeleteSoon() here, see
  // http://crbug.com/701627.
  // It is currently unsafe because |video_encoder_| might be in use on another
  // function on |encoding_task_runner_|, see http://crbug.com/701030.
  encoding_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VEAEncoder::DestroyOnEncodingTaskRunner,
                                base::Unretained(this), &release_waiter));
  release_waiter.Wait();
}

void VEAEncoder::RequireBitstreamBuffers(unsigned int /*input_count*/,
                                         const gfx::Size& input_coded_size,
                                         size_t output_buffer_size) {
  DVLOG(3) << __func__;
  DCHECK(encoding_task_runner_->BelongsToCurrentThread());

  vea_requested_input_coded_size_ = input_coded_size;
  output_buffers_.clear();
  base::queue<std::unique_ptr<base::SharedMemory>>().swap(input_buffers_);

  for (int i = 0; i < kVEAEncoderOutputBufferCount; ++i) {
    std::unique_ptr<base::SharedMemory> shm =
        gpu_factories_->CreateSharedMemory(output_buffer_size);
    if (shm)
      output_buffers_.push_back(base::WrapUnique(shm.release()));
  }

  for (size_t i = 0; i < output_buffers_.size(); ++i)
    UseOutputBitstreamBufferId(i);
}

void VEAEncoder::BitstreamBufferReady(int32_t bitstream_buffer_id,
                                      size_t payload_size,
                                      bool keyframe,
                                      base::TimeDelta timestamp) {
  DVLOG(3) << __func__;
  DCHECK(encoding_task_runner_->BelongsToCurrentThread());

  num_frames_after_keyframe_ = keyframe ? 0 : num_frames_after_keyframe_ + 1;
  if (num_frames_after_keyframe_ > kMaxKeyframeInterval) {
    force_next_frame_to_be_keyframe_ = true;
    num_frames_after_keyframe_ = 0;
  }

  base::SharedMemory* output_buffer =
      output_buffers_[bitstream_buffer_id].get();
  std::unique_ptr<std::string> data(new std::string);
  data->append(reinterpret_cast<char*>(output_buffer->memory()), payload_size);

  const auto front_frame = frames_in_encode_.front();
  frames_in_encode_.pop();
  origin_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(OnFrameEncodeCompleted, on_encoded_video_callback_,
                     front_frame.first, std::move(data), nullptr,
                     front_frame.second, keyframe));
  UseOutputBitstreamBufferId(bitstream_buffer_id);
}

void VEAEncoder::NotifyError(media::VideoEncodeAccelerator::Error error) {
  DVLOG(3) << __func__;
  DCHECK(encoding_task_runner_->BelongsToCurrentThread());
  UMA_HISTOGRAM_ENUMERATION("Media.MediaRecorder.VEAError", error,
                            media::VideoEncodeAccelerator::kErrorMax + 1);
  on_error_callback_.Run();
  error_notified_ = true;
}

void VEAEncoder::UseOutputBitstreamBufferId(int32_t bitstream_buffer_id) {
  DVLOG(3) << __func__;
  DCHECK(encoding_task_runner_->BelongsToCurrentThread());

  video_encoder_->UseOutputBitstreamBuffer(media::BitstreamBuffer(
      bitstream_buffer_id, output_buffers_[bitstream_buffer_id]->handle(),
      output_buffers_[bitstream_buffer_id]->mapped_size()));
}

void VEAEncoder::FrameFinished(std::unique_ptr<base::SharedMemory> shm) {
  DVLOG(3) << __func__;
  DCHECK(encoding_task_runner_->BelongsToCurrentThread());
  input_buffers_.push(std::move(shm));
}

void VEAEncoder::EncodeOnEncodingTaskRunner(scoped_refptr<VideoFrame> frame,
                                            base::TimeTicks capture_timestamp) {
  DVLOG(3) << __func__;
  DCHECK(encoding_task_runner_->BelongsToCurrentThread());

  if (input_visible_size_ != frame->visible_rect().size() && video_encoder_)
    video_encoder_.reset();

  if (!video_encoder_)
    ConfigureEncoderOnEncodingTaskRunner(frame->visible_rect().size());

  if (error_notified_) {
    DVLOG(3) << "An error occurred in VEA encoder";
    return;
  }

  // Drop frames if RequireBitstreamBuffers() hasn't been called.
  if (output_buffers_.empty() || vea_requested_input_coded_size_.IsEmpty()) {
    // TODO(emircan): Investigate if resetting encoder would help.
    DVLOG(3) << "Might drop frame.";
    last_frame_.reset(new std::pair<scoped_refptr<VideoFrame>, base::TimeTicks>(
        frame, capture_timestamp));
    return;
  }

  // If first frame hasn't been encoded, do it first.
  if (last_frame_) {
    std::unique_ptr<VideoFrameAndTimestamp> last_frame(last_frame_.release());
    EncodeOnEncodingTaskRunner(last_frame->first, last_frame->second);
  }

  // Lower resolutions may fall back to SW encoder in some platforms, i.e. Mac.
  // In that case, the encoder expects more frames before returning result.
  // Therefore, a copy is necessary to release the current frame.
  // Only STORAGE_SHMEM backed frames can be shared with GPU process, therefore
  // a copy is required for other storage types.
  scoped_refptr<media::VideoFrame> video_frame = frame;
  if (video_frame->storage_type() != VideoFrame::STORAGE_SHMEM ||
      vea_requested_input_coded_size_ != frame->coded_size() ||
      input_visible_size_.width() < kVEAEncoderMinResolutionWidth ||
      input_visible_size_.height() < kVEAEncoderMinResolutionHeight) {
    // Create SharedMemory backed input buffers as necessary. These SharedMemory
    // instances will be shared with GPU process.
    std::unique_ptr<base::SharedMemory> input_buffer;
    const size_t desired_mapped_size = media::VideoFrame::AllocationSize(
        media::PIXEL_FORMAT_I420, vea_requested_input_coded_size_);
    if (input_buffers_.empty()) {
      input_buffer = gpu_factories_->CreateSharedMemory(desired_mapped_size);
    } else {
      do {
        input_buffer = std::move(input_buffers_.front());
        input_buffers_.pop();
      } while (!input_buffers_.empty() &&
               input_buffer->mapped_size() < desired_mapped_size);
      if (!input_buffer || input_buffer->mapped_size() < desired_mapped_size)
        return;
    }

    video_frame = media::VideoFrame::WrapExternalSharedMemory(
        media::PIXEL_FORMAT_I420, vea_requested_input_coded_size_,
        gfx::Rect(input_visible_size_), input_visible_size_,
        reinterpret_cast<uint8_t*>(input_buffer->memory()),
        input_buffer->mapped_size(), input_buffer->handle(), 0,
        frame->timestamp());
    if (!video_frame) {
      NotifyError(media::VideoEncodeAccelerator::kPlatformFailureError);
      return;
    }
    video_frame->AddDestructionObserver(media::BindToCurrentLoop(
        base::Bind(&VEAEncoder::FrameFinished, this,
                   base::Passed(std::move(input_buffer)))));
    libyuv::I420Copy(frame->visible_data(media::VideoFrame::kYPlane),
                     frame->stride(media::VideoFrame::kYPlane),
                     frame->visible_data(media::VideoFrame::kUPlane),
                     frame->stride(media::VideoFrame::kUPlane),
                     frame->visible_data(media::VideoFrame::kVPlane),
                     frame->stride(media::VideoFrame::kVPlane),
                     video_frame->visible_data(media::VideoFrame::kYPlane),
                     video_frame->stride(media::VideoFrame::kYPlane),
                     video_frame->visible_data(media::VideoFrame::kUPlane),
                     video_frame->stride(media::VideoFrame::kUPlane),
                     video_frame->visible_data(media::VideoFrame::kVPlane),
                     video_frame->stride(media::VideoFrame::kVPlane),
                     input_visible_size_.width(), input_visible_size_.height());
  }
  frames_in_encode_.push(std::make_pair(
      media::WebmMuxer::VideoParameters(frame), capture_timestamp));

  video_encoder_->Encode(video_frame, force_next_frame_to_be_keyframe_);
  force_next_frame_to_be_keyframe_ = false;
}

void VEAEncoder::ConfigureEncoderOnEncodingTaskRunner(const gfx::Size& size) {
  DVLOG(3) << __func__;
  DCHECK(encoding_task_runner_->BelongsToCurrentThread());
  DCHECK(gpu_factories_->GetTaskRunner()->BelongsToCurrentThread());
  DCHECK_GT(bits_per_second_, 0);

  input_visible_size_ = size;
  vea_requested_input_coded_size_ = gfx::Size();
  video_encoder_ = gpu_factories_->CreateVideoEncodeAccelerator();
  if (!video_encoder_ ||
      !video_encoder_->Initialize(media::PIXEL_FORMAT_I420, input_visible_size_,
                                  codec_, bits_per_second_, this)) {
    NotifyError(media::VideoEncodeAccelerator::kPlatformFailureError);
  }
}

void VEAEncoder::DestroyOnEncodingTaskRunner(
    base::WaitableEvent* async_waiter) {
  DCHECK(encoding_task_runner_->BelongsToCurrentThread());
  video_encoder_.reset();
  async_waiter->Signal();
}

}  // namespace content
