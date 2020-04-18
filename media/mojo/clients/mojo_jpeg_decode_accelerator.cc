// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_jpeg_decode_accelerator.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/shared_memory_handle.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

MojoJpegDecodeAccelerator::MojoJpegDecodeAccelerator(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    mojom::JpegDecodeAcceleratorPtrInfo jpeg_decoder)
    : io_task_runner_(std::move(io_task_runner)),
      jpeg_decoder_info_(std::move(jpeg_decoder)) {}

MojoJpegDecodeAccelerator::~MojoJpegDecodeAccelerator() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
}

bool MojoJpegDecodeAccelerator::Initialize(
    JpegDecodeAccelerator::Client* /*client*/) {
  NOTIMPLEMENTED();
  return false;
}

void MojoJpegDecodeAccelerator::InitializeAsync(Client* client,
                                                InitCB init_cb) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  jpeg_decoder_.Bind(std::move(jpeg_decoder_info_));

  // base::Unretained is safe because |this| owns |jpeg_decoder_|.
  jpeg_decoder_.set_connection_error_handler(
      base::Bind(&MojoJpegDecodeAccelerator::OnLostConnectionToJpegDecoder,
                 base::Unretained(this)));
  jpeg_decoder_->Initialize(
      base::Bind(&MojoJpegDecodeAccelerator::OnInitializeDone,
                 base::Unretained(this), std::move(init_cb), client));
}

void MojoJpegDecodeAccelerator::Decode(
    const BitstreamBuffer& bitstream_buffer,
    const scoped_refptr<VideoFrame>& video_frame) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(jpeg_decoder_.is_bound());

  DCHECK(
      base::SharedMemory::IsHandleValid(video_frame->shared_memory_handle()));

  base::SharedMemoryHandle output_handle =
      base::SharedMemory::DuplicateHandle(video_frame->shared_memory_handle());
  if (!base::SharedMemory::IsHandleValid(output_handle)) {
    DLOG(ERROR) << "Failed to duplicate handle of VideoFrame";
    return;
  }

  size_t output_buffer_size = VideoFrame::AllocationSize(
      video_frame->format(), video_frame->coded_size());
  mojo::ScopedSharedBufferHandle output_frame_handle =
      mojo::WrapSharedMemoryHandle(
          output_handle, output_buffer_size,
          mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);

  // base::Unretained is safe because |this| owns |jpeg_decoder_|.
  jpeg_decoder_->Decode(bitstream_buffer, video_frame->coded_size(),
                        std::move(output_frame_handle),
                        base::checked_cast<uint32_t>(output_buffer_size),
                        base::Bind(&MojoJpegDecodeAccelerator::OnDecodeAck,
                                   base::Unretained(this)));
}

bool MojoJpegDecodeAccelerator::IsSupported() {
  return true;
}

void MojoJpegDecodeAccelerator::OnInitializeDone(
    InitCB init_cb,
    JpegDecodeAccelerator::Client* client,
    bool success) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  if (success)
    client_ = client;

  std::move(init_cb).Run(success);
}

void MojoJpegDecodeAccelerator::OnDecodeAck(
    int32_t bitstream_buffer_id,
    ::media::JpegDecodeAccelerator::Error error) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  if (!client_)
    return;

  if (error == ::media::JpegDecodeAccelerator::Error::NO_ERRORS) {
    client_->VideoFrameReady(bitstream_buffer_id);
    return;
  }

  // Only NotifyError once.
  // Client::NotifyError() may trigger deletion of |this|, so calling it needs
  // to be the last thing done on this stack!
  Client* client = nullptr;
  std::swap(client, client_);
  client->NotifyError(bitstream_buffer_id, error);
}

void MojoJpegDecodeAccelerator::OnLostConnectionToJpegDecoder() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  OnDecodeAck(kInvalidBitstreamBufferId,
              ::media::JpegDecodeAccelerator::Error::PLATFORM_FAILURE);
}

}  // namespace media
