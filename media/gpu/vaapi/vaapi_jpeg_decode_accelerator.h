// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_VAAPI_VAAPI_JPEG_DECODE_ACCELERATOR_H_
#define MEDIA_GPU_VAAPI_VAAPI_JPEG_DECODE_ACCELERATOR_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "media/base/bitstream_buffer.h"
#include "media/gpu/media_gpu_export.h"
#include "media/gpu/shared_memory_region.h"
#include "media/gpu/vaapi/vaapi_jpeg_decoder.h"
#include "media/gpu/vaapi/vaapi_wrapper.h"
#include "media/video/jpeg_decode_accelerator.h"

namespace media {

// Class to provide JPEG decode acceleration for Intel systems with hardware
// support for it, and on which libva is available.
// Decoding tasks are performed in a separate decoding thread.
//
// Threading/life-cycle: this object is created & destroyed on the GPU
// ChildThread.  A few methods on it are called on the decoder thread which is
// stopped during |this->Destroy()|, so any tasks posted to the decoder thread
// can assume |*this| is still alive.  See |weak_this_| below for more details.
class MEDIA_GPU_EXPORT VaapiJpegDecodeAccelerator
    : public JpegDecodeAccelerator {
 public:
  VaapiJpegDecodeAccelerator(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);
  ~VaapiJpegDecodeAccelerator() override;

  // JpegDecodeAccelerator implementation.
  bool Initialize(JpegDecodeAccelerator::Client* client) override;
  void Decode(const BitstreamBuffer& bitstream_buffer,
              const scoped_refptr<VideoFrame>& video_frame) override;
  bool IsSupported() override;

 private:
  // An input buffer and the corresponding output video frame awaiting
  // consumption, provided by the client.
  struct DecodeRequest {
    DecodeRequest(int32_t bitstream_buffer_id,
                  std::unique_ptr<SharedMemoryRegion> shm,
                  const scoped_refptr<VideoFrame>& video_frame);
    ~DecodeRequest();

    int32_t bitstream_buffer_id;
    std::unique_ptr<SharedMemoryRegion> shm;
    scoped_refptr<VideoFrame> video_frame;
  };

  // Notifies the client that an error has occurred and decoding cannot
  // continue.
  void NotifyError(int32_t bitstream_buffer_id, Error error);
  void NotifyErrorFromDecoderThread(int32_t bitstream_buffer_id, Error error);
  void VideoFrameReady(int32_t bitstream_buffer_id);

  // Processes one decode |request|.
  void DecodeTask(const std::unique_ptr<DecodeRequest>& request);

  // Puts contents of |va_surface| into given |video_frame|, releases the
  // surface and passes the |input_buffer_id| of the resulting picture to
  // client for output.
  bool OutputPicture(VASurfaceID va_surface_id,
                     int32_t input_buffer_id,
                     const scoped_refptr<VideoFrame>& video_frame);

  // ChildThread's task runner.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // GPU IO task runner.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // The client of this class.
  Client* client_;

  // WeakPtr<> pointing to |this| for use in posting tasks from the decoder
  // thread back to the ChildThread.  Because the decoder thread is a member of
  // this class, any task running on the decoder thread is guaranteed that this
  // object is still alive.  As a result, tasks posted from ChildThread to
  // decoder thread should use base::Unretained(this), and tasks posted from the
  // decoder thread to the ChildThread should use |weak_this_|.
  base::WeakPtr<VaapiJpegDecodeAccelerator> weak_this_;

  scoped_refptr<VaapiWrapper> vaapi_wrapper_;

  // Comes after vaapi_wrapper_ to ensure its destructor is executed before
  // |vaapi_wrapper_| is destroyed.
  std::unique_ptr<VaapiJpegDecoder> decoder_;
  base::Thread decoder_thread_;
  // Use this to post tasks to |decoder_thread_| instead of
  // |decoder_thread_.task_runner()| because the latter will be NULL once
  // |decoder_thread_.Stop()| returns.
  scoped_refptr<base::SingleThreadTaskRunner> decoder_task_runner_;

  // The current VA surface for decoding.
  VASurfaceID va_surface_id_;
  // The coded size associated with |va_surface_id_|.
  gfx::Size coded_size_;
  // The VA RT format associated with |va_surface_id_|.
  unsigned int va_rt_format_;

  // The WeakPtrFactory for |weak_this_|.
  base::WeakPtrFactory<VaapiJpegDecodeAccelerator> weak_this_factory_;

  DISALLOW_COPY_AND_ASSIGN(VaapiJpegDecodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_GPU_VAAPI_VAAPI_JPEG_DECODE_ACCELERATOR_H_
