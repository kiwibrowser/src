// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_GPU_JPEG_DECODER_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_GPU_JPEG_DECODER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "content/common/content_export.h"
#include "gpu/config/gpu_info.h"
#include "media/capture/video/video_capture_jpeg_decoder.h"
#include "media/mojo/clients/mojo_jpeg_decode_accelerator.h"

namespace base {
class WaitableEvent;
}

namespace content {

// Adapter to GpuJpegDecodeAccelerator for VideoCaptureDevice::Client. It takes
// care of GpuJpegDecodeAccelerator creation, shared memory, and threading
// issues.
//
// All public methods except JpegDecodeAccelerator::Client ones should be called
// on the same thread. JpegDecodeAccelerator::Client methods should be called on
// the IO thread.
class CONTENT_EXPORT VideoCaptureGpuJpegDecoder
    : public media::VideoCaptureJpegDecoder,
      public media::JpegDecodeAccelerator::Client {
 public:
  // |decode_done_cb| is called on the IO thread when decode succeeds. This can
  // be on any thread. |decode_done_cb| is never called after
  // VideoCaptureGpuJpegDecoder is destroyed.
  VideoCaptureGpuJpegDecoder(
      DecodeDoneCB decode_done_cb,
      base::Callback<void(const std::string&)> send_log_message_cb);
  ~VideoCaptureGpuJpegDecoder() override;

  // Implementation of VideoCaptureJpegDecoder:
  void Initialize() override;
  STATUS GetStatus() const override;
  void DecodeCapturedData(
      const uint8_t* data,
      size_t in_buffer_size,
      const media::VideoCaptureFormat& frame_format,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      media::VideoCaptureDevice::Client::Buffer out_buffer) override;

  // JpegDecodeAccelerator::Client implementation.
  // These will be called on IO thread.
  void VideoFrameReady(int32_t buffer_id) override;
  void NotifyError(int32_t buffer_id,
                   media::JpegDecodeAccelerator::Error error) override;

 private:
  static void RequestGPUInfoOnIOThread(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      base::WeakPtr<VideoCaptureGpuJpegDecoder> weak_this);

  static void DidReceiveGPUInfoOnIOThread(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      base::WeakPtr<VideoCaptureGpuJpegDecoder> weak_this,
      const gpu::GPUInfo& gpu_info);

  void FinishInitialization(
      media::mojom::JpegDecodeAcceleratorPtrInfo unbound_remote_decoder);
  void OnInitializationDone(bool success);

  // Returns true if the decoding of last frame is not finished yet.
  bool IsDecoding_Locked() const;

  // Records |decoder_status_| to histogram.
  void RecordInitDecodeUMA_Locked();

  void DestroyDecoderOnIOThread(base::WaitableEvent* event);

  // The underlying JPEG decode accelerator.
  std::unique_ptr<media::JpegDecodeAccelerator> decoder_;

  // The callback to run when decode succeeds.
  const DecodeDoneCB decode_done_cb_;

  const base::Callback<void(const std::string&)> send_log_message_cb_;
  bool has_received_decoded_frame_;

  // Guards |decode_done_closure_| and |decoder_status_|.
  mutable base::Lock lock_;

  // The closure of |decode_done_cb_| with bound parameters.
  base::Closure decode_done_closure_;

  // Next id for input BitstreamBuffer.
  int32_t next_bitstream_buffer_id_;

  // The id for current input BitstreamBuffer being decoded.
  int32_t in_buffer_id_;

  // Shared memory to store JPEG stream buffer. The input BitstreamBuffer is
  // backed by this.
  std::unique_ptr<base::SharedMemory> in_shared_memory_;

  STATUS decoder_status_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<VideoCaptureGpuJpegDecoder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureGpuJpegDecoder);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_VIDEO_CAPTURE_GPU_JPEG_DECODER_H_
