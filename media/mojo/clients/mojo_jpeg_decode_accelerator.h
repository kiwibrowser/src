// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_JPEG_DECODE_ACCELERATOR_H_
#define MEDIA_MOJO_CLIENTS_MOJO_JPEG_DECODE_ACCELERATOR_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "media/mojo/interfaces/jpeg_decode_accelerator.mojom.h"
#include "media/video/jpeg_decode_accelerator.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

// A JpegDecodeAccelerator, for use in the browser process, that proxies to a
// mojom::JpegDecodeAccelerator. Created on the owner's thread, otherwise
// operating and deleted on the IO thread.
class MojoJpegDecodeAccelerator : public JpegDecodeAccelerator {
 public:
  MojoJpegDecodeAccelerator(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      mojom::JpegDecodeAcceleratorPtrInfo jpeg_decoder);
  ~MojoJpegDecodeAccelerator() override;

  // JpegDecodeAccelerator implementation.
  // |client| is called on the IO thread, but is never called into after the
  // MojoJpegDecodeAccelerator is destroyed.
  bool Initialize(Client* client) override;
  void InitializeAsync(Client* client, InitCB init_cb) override;
  void Decode(const BitstreamBuffer& bitstream_buffer,
              const scoped_refptr<VideoFrame>& video_frame) override;
  bool IsSupported() override;

 private:
  void OnInitializeDone(InitCB init_cb,
                        JpegDecodeAccelerator::Client* client,
                        bool success);
  void OnDecodeAck(int32_t bitstream_buffer_id,
                   ::media::JpegDecodeAccelerator::Error error);
  void OnLostConnectionToJpegDecoder();

  // Browser IO task runner.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  Client* client_ = nullptr;

  // Used to safely pass the mojom::JpegDecodeAcceleratorPtr from one thread to
  // another. It is set in the constructor and consumed in InitializeAsync().
  // TODO(mcasas): s/jpeg_decoder_/jda_/ https://crbug.com/699255.
  mojom::JpegDecodeAcceleratorPtrInfo jpeg_decoder_info_;

  mojom::JpegDecodeAcceleratorPtr jpeg_decoder_;

  DISALLOW_COPY_AND_ASSIGN(MojoJpegDecodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_JPEG_DECODE_ACCELERATOR_H_
