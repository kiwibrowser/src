// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_FAKE_VIDEO_DECODE_ACCELERATOR_H_
#define MEDIA_GPU_FAKE_VIDEO_DECODE_ACCELERATOR_H_

#include <stdint.h>

#include <vector>

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/gpu/gpu_video_decode_accelerator_helpers.h"
#include "media/gpu/media_gpu_export.h"
#include "media/video/video_decode_accelerator.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gl/gl_context.h"

namespace media {

class MEDIA_GPU_EXPORT FakeVideoDecodeAccelerator
    : public VideoDecodeAccelerator {
 public:
  FakeVideoDecodeAccelerator(
      const gfx::Size& size,
      const MakeGLContextCurrentCallback& make_context_current_cb);
  ~FakeVideoDecodeAccelerator() override;

  bool Initialize(const Config& config, Client* client) override;
  void Decode(const BitstreamBuffer& bitstream_buffer) override;
  void AssignPictureBuffers(const std::vector<PictureBuffer>& buffers) override;
  void ReusePictureBuffer(int32_t picture_buffer_id) override;
  void Flush() override;
  void Reset() override;
  void Destroy() override;
  bool TryToSetupDecodeOnSeparateThread(
      const base::WeakPtr<Client>& decode_client,
      const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner)
      override;

 private:
  void DoPictureReady();

  // The message loop that created the class. Used for all callbacks. This
  // class expects all calls to this class to be on this message loop (not
  // checked).
  const scoped_refptr<base::SingleThreadTaskRunner> child_task_runner_;

  Client* client_;

  // Make our context current before running any GL entry points.
  MakeGLContextCurrentCallback make_context_current_cb_;

  // Output picture size.
  gfx::Size frame_buffer_size_;

  // Picture buffer ids that are available for putting fake frames in.
  base::queue<int> free_output_buffers_;
  // BitstreamBuffer ids for buffers that contain new data to decode.
  base::queue<int> queued_bitstream_ids_;

  bool flushing_;

  // The WeakPtrFactory for |weak_this_|.
  base::WeakPtrFactory<FakeVideoDecodeAccelerator> weak_this_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakeVideoDecodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_GPU_FAKE_VIDEO_DECODE_ACCELERATOR_H_
