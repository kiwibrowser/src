// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_jpeg_decode_accelerator_service.h"

#include <stdint.h>

#include <memory>
#include <utility>

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

void DecodeFinished(std::unique_ptr<base::SharedMemory> shm) {
  // Do nothing. Because VideoFrame is backed by |shm|, the purpose of this
  // function is to just keep reference of |shm| to make sure it lives until
  // decode finishes.
}

bool VerifyDecodeParams(const gfx::Size& coded_size,
                        mojo::ScopedSharedBufferHandle* output_handle,
                        uint32_t output_buffer_size) {
  const int kJpegMaxDimension = UINT16_MAX;
  if (coded_size.IsEmpty() || coded_size.width() > kJpegMaxDimension ||
      coded_size.height() > kJpegMaxDimension) {
    LOG(ERROR) << "invalid coded_size " << coded_size.ToString();
    return false;
  }

  if (!output_handle->is_valid()) {
    LOG(ERROR) << "invalid output_handle";
    return false;
  }

  uint32_t allocation_size =
      media::VideoFrame::AllocationSize(media::PIXEL_FORMAT_I420, coded_size);
  if (output_buffer_size < allocation_size) {
    DLOG(ERROR) << "output_buffer_size is too small: " << output_buffer_size
                << ". It needs: " << allocation_size;
    return false;
  }

  return true;
}

}  // namespace

namespace media {

// static
void MojoJpegDecodeAcceleratorService::Create(
    mojom::JpegDecodeAcceleratorRequest request) {
  auto* jpeg_decoder = new MojoJpegDecodeAcceleratorService();
  mojo::MakeStrongBinding(base::WrapUnique(jpeg_decoder), std::move(request));
}

MojoJpegDecodeAcceleratorService::MojoJpegDecodeAcceleratorService()
    : accelerator_factory_functions_(
          GpuJpegDecodeAcceleratorFactory::GetAcceleratorFactories()) {}

MojoJpegDecodeAcceleratorService::~MojoJpegDecodeAcceleratorService() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void MojoJpegDecodeAcceleratorService::VideoFrameReady(
    int32_t bitstream_buffer_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NotifyDecodeStatus(bitstream_buffer_id,
                     ::media::JpegDecodeAccelerator::Error::NO_ERRORS);
}

void MojoJpegDecodeAcceleratorService::NotifyError(
    int32_t bitstream_buffer_id,
    ::media::JpegDecodeAccelerator::Error error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NotifyDecodeStatus(bitstream_buffer_id, error);
}

void MojoJpegDecodeAcceleratorService::Initialize(InitializeCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // When adding non-chromeos platforms, VideoCaptureGpuJpegDecoder::Initialize
  // needs to be updated.

  std::unique_ptr<::media::JpegDecodeAccelerator> accelerator;
  for (const auto& create_jda_function : accelerator_factory_functions_) {
    std::unique_ptr<::media::JpegDecodeAccelerator> tmp_accelerator =
        create_jda_function.Run(base::ThreadTaskRunnerHandle::Get());
    if (tmp_accelerator && tmp_accelerator->Initialize(this)) {
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

void MojoJpegDecodeAcceleratorService::Decode(
    const BitstreamBuffer& input_buffer,
    const gfx::Size& coded_size,
    mojo::ScopedSharedBufferHandle output_handle,
    uint32_t output_buffer_size,
    DecodeCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  TRACE_EVENT0("jpeg", "MojoJpegDecodeAcceleratorService::Decode");

  DCHECK_EQ(decode_cb_map_.count(input_buffer.id()), 0u);
  decode_cb_map_[input_buffer.id()] = std::move(callback);

  if (!VerifyDecodeParams(coded_size, &output_handle, output_buffer_size)) {
    NotifyDecodeStatus(input_buffer.id(),
                       ::media::JpegDecodeAccelerator::Error::INVALID_ARGUMENT);
    return;
  }

  base::SharedMemoryHandle memory_handle;
  MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(output_handle), &memory_handle, nullptr, nullptr);
  DCHECK_EQ(MOJO_RESULT_OK, result);

  std::unique_ptr<base::SharedMemory> output_shm(
      new base::SharedMemory(memory_handle, false));
  if (!output_shm->Map(output_buffer_size)) {
    LOG(ERROR) << "Could not map output shared memory for input buffer id "
               << input_buffer.id();
    NotifyDecodeStatus(input_buffer.id(),
                       ::media::JpegDecodeAccelerator::Error::PLATFORM_FAILURE);
    return;
  }

  uint8_t* shm_memory = static_cast<uint8_t*>(output_shm->memory());
  scoped_refptr<VideoFrame> frame = VideoFrame::WrapExternalSharedMemory(
      PIXEL_FORMAT_I420,      // format
      coded_size,             // coded_size
      gfx::Rect(coded_size),  // visible_rect
      coded_size,             // natural_size
      shm_memory,             // data
      output_buffer_size,     // data_size
      memory_handle,          // handle
      0,                      // data_offset
      base::TimeDelta());     // timestamp
  if (!frame.get()) {
    LOG(ERROR) << "Could not create VideoFrame for input buffer id "
               << input_buffer.id();
    NotifyDecodeStatus(input_buffer.id(),
                       ::media::JpegDecodeAccelerator::Error::PLATFORM_FAILURE);
    return;
  }
  frame->AddDestructionObserver(
      base::Bind(DecodeFinished, base::Passed(&output_shm)));

  DCHECK(accelerator_);
  accelerator_->Decode(input_buffer, frame);
}

void MojoJpegDecodeAcceleratorService::DecodeWithFD(
    int32_t buffer_id,
    mojo::ScopedHandle input_handle,
    uint32_t input_buffer_size,
    int32_t coded_size_width,
    int32_t coded_size_height,
    mojo::ScopedHandle output_handle,
    uint32_t output_buffer_size,
    DecodeWithFDCallback callback) {
#if defined(OS_CHROMEOS)
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  base::PlatformFile input_fd;
  base::PlatformFile output_fd;
  MojoResult result;

  result = mojo::UnwrapPlatformFile(std::move(input_handle), &input_fd);
  if (result != MOJO_RESULT_OK) {
    std::move(callback).Run(
        buffer_id, ::media::JpegDecodeAccelerator::Error::PLATFORM_FAILURE);
    return;
  }

  result = mojo::UnwrapPlatformFile(std::move(output_handle), &output_fd);
  if (result != MOJO_RESULT_OK) {
    std::move(callback).Run(
        buffer_id, ::media::JpegDecodeAccelerator::Error::PLATFORM_FAILURE);
    return;
  }

  base::UnguessableToken guid = base::UnguessableToken::Create();
  base::SharedMemoryHandle input_shm_handle(
      base::FileDescriptor(input_fd, true), 0u, guid);
  base::SharedMemoryHandle output_shm_handle(
      base::FileDescriptor(output_fd, true), 0u, guid);

  media::BitstreamBuffer in_buffer(buffer_id, input_shm_handle,
                                   input_buffer_size);
  gfx::Size coded_size(coded_size_width, coded_size_height);

  mojo::ScopedSharedBufferHandle output_scoped_handle =
      mojo::WrapSharedMemoryHandle(
          output_shm_handle, output_buffer_size,
          mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);

  Decode(in_buffer, coded_size, std::move(output_scoped_handle),
         output_buffer_size, std::move(callback));
#else
  NOTREACHED();
#endif
}

void MojoJpegDecodeAcceleratorService::Uninitialize() {
  // TODO(c.padhi): see http://crbug.com/699255.
  NOTIMPLEMENTED();
}

void MojoJpegDecodeAcceleratorService::NotifyDecodeStatus(
    int32_t bitstream_buffer_id,
    ::media::JpegDecodeAccelerator::Error error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  auto iter = decode_cb_map_.find(bitstream_buffer_id);
  DCHECK(iter != decode_cb_map_.end());
  DecodeCallback decode_cb = std::move(iter->second);
  decode_cb_map_.erase(iter);
  std::move(decode_cb).Run(bitstream_buffer_id, error);
}

}  // namespace media
