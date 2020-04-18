// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_jpeg_encode_accelerator_service.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/shared_memory.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "media/base/bind_to_current_loop.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ui/gfx/geometry/size.h"

namespace {

#if defined(OS_CHROMEOS)
const int kJpegQuality = 90;
#endif

}  // namespace

namespace media {

// static
void MojoJpegEncodeAcceleratorService::Create(
    mojom::JpegEncodeAcceleratorRequest request) {
  auto* jpeg_encoder = new MojoJpegEncodeAcceleratorService();
  mojo::MakeStrongBinding(base::WrapUnique(jpeg_encoder), std::move(request));
}

MojoJpegEncodeAcceleratorService::MojoJpegEncodeAcceleratorService()
    : accelerator_factory_functions_(
          GpuJpegEncodeAcceleratorFactory::GetAcceleratorFactories()) {}

MojoJpegEncodeAcceleratorService::~MojoJpegEncodeAcceleratorService() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void MojoJpegEncodeAcceleratorService::VideoFrameReady(
    int32_t bitstream_buffer_id,
    size_t encoded_picture_size) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NotifyEncodeStatus(bitstream_buffer_id, encoded_picture_size,
                     ::media::JpegEncodeAccelerator::Status::ENCODE_OK);
}

void MojoJpegEncodeAcceleratorService::NotifyError(
    int32_t bitstream_buffer_id,
    ::media::JpegEncodeAccelerator::Status error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NotifyEncodeStatus(bitstream_buffer_id, 0, error);
}

void MojoJpegEncodeAcceleratorService::Initialize(InitializeCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // When adding non-chromeos platforms, VideoCaptureGpuJpegEncoder::Initialize
  // needs to be updated.

  std::unique_ptr<::media::JpegEncodeAccelerator> accelerator;
  for (const auto& create_jea_function : accelerator_factory_functions_) {
    std::unique_ptr<::media::JpegEncodeAccelerator> tmp_accelerator =
        create_jea_function.Run(base::ThreadTaskRunnerHandle::Get());
    if (tmp_accelerator &&
        tmp_accelerator->Initialize(this) ==
            ::media::JpegEncodeAccelerator::Status::ENCODE_OK) {
      accelerator = std::move(tmp_accelerator);
      break;
    }
  }

  if (!accelerator) {
    DLOG(ERROR) << "JPEG accelerator initialization failed";
    std::move(callback).Run(false);
    return;
  }

  accelerator_ = std::move(accelerator);
  std::move(callback).Run(true);
}

void MojoJpegEncodeAcceleratorService::EncodeWithFD(
    int32_t buffer_id,
    mojo::ScopedHandle input_handle,
    uint32_t input_buffer_size,
    int32_t coded_size_width,
    int32_t coded_size_height,
    mojo::ScopedHandle exif_handle,
    uint32_t exif_buffer_size,
    mojo::ScopedHandle output_handle,
    uint32_t output_buffer_size,
    EncodeWithFDCallback callback) {
#if defined(OS_CHROMEOS)
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  base::PlatformFile input_fd;
  base::PlatformFile exif_fd;
  base::PlatformFile output_fd;
  MojoResult result;

  if (coded_size_width <= 0 || coded_size_height <= 0) {
    std::move(callback).Run(
        buffer_id, 0, ::media::JpegEncodeAccelerator::Status::INVALID_ARGUMENT);
    return;
  }

  result = mojo::UnwrapPlatformFile(std::move(input_handle), &input_fd);
  if (result != MOJO_RESULT_OK) {
    std::move(callback).Run(
        buffer_id, 0, ::media::JpegEncodeAccelerator::Status::PLATFORM_FAILURE);
    return;
  }

  result = mojo::UnwrapPlatformFile(std::move(exif_handle), &exif_fd);
  if (result != MOJO_RESULT_OK) {
    std::move(callback).Run(
        buffer_id, 0, ::media::JpegEncodeAccelerator::Status::PLATFORM_FAILURE);
    return;
  }

  result = mojo::UnwrapPlatformFile(std::move(output_handle), &output_fd);
  if (result != MOJO_RESULT_OK) {
    std::move(callback).Run(
        buffer_id, 0, ::media::JpegEncodeAccelerator::Status::PLATFORM_FAILURE);
    return;
  }

  base::UnguessableToken input_guid = base::UnguessableToken::Create();
  base::UnguessableToken exif_guid = base::UnguessableToken::Create();
  base::UnguessableToken output_guid = base::UnguessableToken::Create();
  base::SharedMemoryHandle input_shm_handle(
      base::FileDescriptor(input_fd, true), 0u, input_guid);
  base::SharedMemoryHandle exif_shm_handle(base::FileDescriptor(exif_fd, true),
                                           0u, exif_guid);
  base::SharedMemoryHandle output_shm_handle(
      base::FileDescriptor(output_fd, true), 0u, output_guid);

  media::BitstreamBuffer output_buffer(buffer_id, output_shm_handle,
                                       output_buffer_size);
  std::unique_ptr<media::BitstreamBuffer> exif_buffer;
  if (exif_buffer_size > 0) {
    exif_buffer = std::make_unique<media::BitstreamBuffer>(
        buffer_id, exif_shm_handle, exif_buffer_size);
  }
  gfx::Size coded_size(coded_size_width, coded_size_height);

  if (encode_cb_map_.find(buffer_id) != encode_cb_map_.end()) {
    mojo::ReportBadMessage("buffer_id is already registered in encode_cb_map_");
    return;
  }
  encode_cb_map_.emplace(buffer_id, std::move(callback));

  auto input_shm = std::make_unique<base::SharedMemory>(input_shm_handle, true);
  if (!input_shm->Map(input_buffer_size)) {
    DLOG(ERROR) << "Could not map input shared memory for buffer id "
                << buffer_id;
    NotifyEncodeStatus(
        buffer_id, 0, ::media::JpegEncodeAccelerator::Status::PLATFORM_FAILURE);
    return;
  }

  uint8_t* input_shm_memory = static_cast<uint8_t*>(input_shm->memory());
  scoped_refptr<VideoFrame> frame = VideoFrame::WrapExternalSharedMemory(
      PIXEL_FORMAT_I420,      // format
      coded_size,             // coded_size
      gfx::Rect(coded_size),  // visible_rect
      coded_size,             // natural_size
      input_shm_memory,       // data
      input_buffer_size,      // data_size
      input_shm_handle,       // handle
      0,                      // data_offset
      base::TimeDelta());     // timestamp
  if (!frame.get()) {
    LOG(ERROR) << "Could not create VideoFrame for buffer id " << buffer_id;
    NotifyEncodeStatus(
        buffer_id, 0, ::media::JpegEncodeAccelerator::Status::PLATFORM_FAILURE);
    return;
  }
  // Keep |input_shm| referenced until |frame| is destructed.
  frame->AddDestructionObserver(base::BindOnce(
      base::DoNothing::Once<std::unique_ptr<base::SharedMemory>>(),
      base::Passed(&input_shm)));

  DCHECK(accelerator_);
  accelerator_->Encode(frame, kJpegQuality, exif_buffer.get(), output_buffer);
#else
  NOTREACHED();
#endif
}

void MojoJpegEncodeAcceleratorService::NotifyEncodeStatus(
    int32_t bitstream_buffer_id,
    size_t encoded_picture_size,
    ::media::JpegEncodeAccelerator::Status error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  auto iter = encode_cb_map_.find(bitstream_buffer_id);
  DCHECK(iter != encode_cb_map_.end());
  EncodeWithFDCallback encode_cb = std::move(iter->second);
  encode_cb_map_.erase(iter);
  std::move(encode_cb).Run(bitstream_buffer_id, encoded_picture_size, error);
}

}  // namespace media
