// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_video_encode_accelerator_service.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/limits.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

// static
void MojoVideoEncodeAcceleratorService::Create(
    mojom::VideoEncodeAcceleratorRequest request,
    const CreateAndInitializeVideoEncodeAcceleratorCallback&
        create_vea_callback,
    const gpu::GpuPreferences& gpu_preferences) {
  mojo::MakeStrongBinding(std::make_unique<MojoVideoEncodeAcceleratorService>(
                              create_vea_callback, gpu_preferences),
                          std::move(request));
}

MojoVideoEncodeAcceleratorService::MojoVideoEncodeAcceleratorService(
    const CreateAndInitializeVideoEncodeAcceleratorCallback&
        create_vea_callback,
    const gpu::GpuPreferences& gpu_preferences)
    : create_vea_callback_(create_vea_callback),
      gpu_preferences_(gpu_preferences),
      output_buffer_size_(0),
      weak_factory_(this) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

MojoVideoEncodeAcceleratorService::~MojoVideoEncodeAcceleratorService() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MojoVideoEncodeAcceleratorService::Initialize(
    VideoPixelFormat input_format,
    const gfx::Size& input_visible_size,
    VideoCodecProfile output_profile,
    uint32_t initial_bitrate,
    mojom::VideoEncodeAcceleratorClientPtr client,
    InitializeCallback success_callback) {
  DVLOG(1) << __func__
           << " input_format=" << VideoPixelFormatToString(input_format)
           << ", input_visible_size=" << input_visible_size.ToString()
           << ", output_profile=" << GetProfileName(output_profile)
           << ", initial_bitrate=" << initial_bitrate;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!encoder_);
  DCHECK_EQ(PIXEL_FORMAT_I420, input_format) << "Only I420 format supported";

  if (!client) {
    DLOG(ERROR) << __func__ << "null |client|";
    std::move(success_callback).Run(false);
    return;
  }
  vea_client_ = std::move(client);

  if (input_visible_size.width() > limits::kMaxDimension ||
      input_visible_size.height() > limits::kMaxDimension ||
      input_visible_size.GetArea() > limits::kMaxCanvas) {
    DLOG(ERROR) << __func__ << "too large input_visible_size "
                << input_visible_size.ToString();
    std::move(success_callback).Run(false);
    return;
  }

  encoder_ =
      create_vea_callback_.Run(input_format, input_visible_size, output_profile,
                               initial_bitrate, this, gpu_preferences_);
  if (!encoder_) {
    DLOG(ERROR) << __func__ << " Error creating or initializing VEA";
    std::move(success_callback).Run(false);
    return;
  }

  std::move(success_callback).Run(true);
  return;
}

void MojoVideoEncodeAcceleratorService::Encode(
    const scoped_refptr<VideoFrame>& frame,
    bool force_keyframe,
    EncodeCallback callback) {
  DVLOG(2) << __func__ << " tstamp=" << frame->timestamp();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!encoder_)
    return;

  if (frame->coded_size() != input_coded_size_) {
    DLOG(ERROR) << __func__ << " wrong input coded size, expected "
                << input_coded_size_.ToString() << ", got "
                << frame->coded_size().ToString();
    NotifyError(::media::VideoEncodeAccelerator::kInvalidArgumentError);
    std::move(callback).Run();
    return;
  }

  frame->AddDestructionObserver(media::BindToCurrentLoop(std::move(callback)));
  encoder_->Encode(frame, force_keyframe);
}

void MojoVideoEncodeAcceleratorService::UseOutputBitstreamBuffer(
    int32_t bitstream_buffer_id,
    mojo::ScopedSharedBufferHandle buffer) {
  DVLOG(2) << __func__ << " bitstream_buffer_id=" << bitstream_buffer_id;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!encoder_)
    return;
  if (!buffer.is_valid()) {
    DLOG(ERROR) << __func__ << " invalid |buffer|.";
    NotifyError(::media::VideoEncodeAccelerator::kInvalidArgumentError);
    return;
  }
  if (bitstream_buffer_id < 0) {
    DLOG(ERROR) << __func__ << " bitstream_buffer_id=" << bitstream_buffer_id
                << " must be >= 0";
    NotifyError(::media::VideoEncodeAccelerator::kInvalidArgumentError);
    return;
  }

  base::SharedMemoryHandle handle;
  size_t memory_size = 0;
  auto result = mojo::UnwrapSharedMemoryHandle(std::move(buffer), &handle,
                                               &memory_size, nullptr);
  if (result != MOJO_RESULT_OK || memory_size == 0u) {
    DLOG(ERROR) << __func__ << " mojo::UnwrapSharedMemoryHandle() failed";
    NotifyError(::media::VideoEncodeAccelerator::kPlatformFailureError);
    return;
  }

  if (memory_size < output_buffer_size_) {
    DLOG(ERROR) << __func__ << " bitstream_buffer_id=" << bitstream_buffer_id
                << " has a size of " << memory_size
                << "B, different from expected " << output_buffer_size_ << "B";
    NotifyError(::media::VideoEncodeAccelerator::kInvalidArgumentError);
    return;
  }

  encoder_->UseOutputBitstreamBuffer(
      BitstreamBuffer(bitstream_buffer_id, handle, memory_size));
}

void MojoVideoEncodeAcceleratorService::RequestEncodingParametersChange(
    uint32_t bitrate,
    uint32_t framerate) {
  DVLOG(2) << __func__ << " bitrate=" << bitrate << " framerate=" << framerate;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!encoder_)
    return;
  encoder_->RequestEncodingParametersChange(bitrate, framerate);
}

void MojoVideoEncodeAcceleratorService::RequireBitstreamBuffers(
    unsigned int input_count,
    const gfx::Size& input_coded_size,
    size_t output_buffer_size) {
  DVLOG(2) << __func__ << " input_count=" << input_count
           << " input_coded_size=" << input_coded_size.ToString()
           << " output_buffer_size=" << output_buffer_size;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!vea_client_)
    return;

  output_buffer_size_ = output_buffer_size;
  input_coded_size_ = input_coded_size;

  vea_client_->RequireBitstreamBuffers(input_count, input_coded_size,
                                       output_buffer_size);
}

void MojoVideoEncodeAcceleratorService::BitstreamBufferReady(
    int32_t bitstream_buffer_id,
    size_t payload_size,
    bool key_frame,
    base::TimeDelta timestamp) {
  DVLOG(2) << __func__ << " bitstream_buffer_id=" << bitstream_buffer_id
           << ", payload_size=" << payload_size
           << "B,  key_frame=" << key_frame;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!vea_client_)
    return;

  vea_client_->BitstreamBufferReady(bitstream_buffer_id, payload_size,
                                    key_frame, timestamp);
}

void MojoVideoEncodeAcceleratorService::NotifyError(
    ::media::VideoEncodeAccelerator::Error error) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!vea_client_)
    return;

  vea_client_->NotifyError(error);
}

}  // namespace media
