// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/video_capture_gpu_jpeg_decoder.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_switches.h"
#include "media/base/video_frame.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/ui/public/cpp/gpu/gpu.h"

namespace content {

VideoCaptureGpuJpegDecoder::VideoCaptureGpuJpegDecoder(
    DecodeDoneCB decode_done_cb,
    base::Callback<void(const std::string&)> send_log_message_cb)
    : decode_done_cb_(std::move(decode_done_cb)),
      send_log_message_cb_(std::move(send_log_message_cb)),
      has_received_decoded_frame_(false),
      next_bitstream_buffer_id_(0),
      in_buffer_id_(media::JpegDecodeAccelerator::kInvalidBitstreamBufferId),
      decoder_status_(INIT_PENDING),
      weak_ptr_factory_(this) {}

VideoCaptureGpuJpegDecoder::~VideoCaptureGpuJpegDecoder() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // |this| was set as |decoder_|'s client. |decoder_| has to be deleted before
  // this destructor returns to ensure that it doesn't call back into its
  // client. Hence, we wait here while we delete |decoder_| on the IO thread.
  if (decoder_) {
    base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
    // base::Unretained is safe because |this| will be valid until |event|
    // is signaled.
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&VideoCaptureGpuJpegDecoder::DestroyDecoderOnIOThread,
                       base::Unretained(this), &event));
    event.Wait();
  }
}

void VideoCaptureGpuJpegDecoder::Initialize() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::AutoLock lock(lock_);
  bool is_platform_supported =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseFakeJpegDecodeAccelerator);
#if defined(OS_CHROMEOS)
  // Non-ChromeOS platforms do not support HW JPEG decode now. Do not establish
  // gpu channel to avoid introducing overhead.
  is_platform_supported = true;
#endif

  if (!is_platform_supported ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableAcceleratedMjpegDecode)) {
    decoder_status_ = FAILED;
    RecordInitDecodeUMA_Locked();
    return;
  }

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(&RequestGPUInfoOnIOThread,
                                         base::ThreadTaskRunnerHandle::Get(),
                                         weak_ptr_factory_.GetWeakPtr()));
}

VideoCaptureGpuJpegDecoder::STATUS VideoCaptureGpuJpegDecoder::GetStatus()
    const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AutoLock lock(lock_);
  return decoder_status_;
}

void VideoCaptureGpuJpegDecoder::DecodeCapturedData(
    const uint8_t* data,
    size_t in_buffer_size,
    const media::VideoCaptureFormat& frame_format,
    base::TimeTicks reference_time,
    base::TimeDelta timestamp,
    media::VideoCaptureDevice::Client::Buffer out_buffer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(decoder_);

  TRACE_EVENT_ASYNC_BEGIN0("jpeg", "VideoCaptureGpuJpegDecoder decoding",
                           next_bitstream_buffer_id_);
  TRACE_EVENT0("jpeg", "VideoCaptureGpuJpegDecoder::DecodeCapturedData");

  // TODO(kcwu): enqueue decode requests in case decoding is not fast enough
  // (say, if decoding time is longer than 16ms for 60fps 4k image)
  {
    base::AutoLock lock(lock_);
    if (IsDecoding_Locked()) {
      DVLOG(1) << "Drop captured frame. Previous jpeg frame is still decoding";
      return;
    }
  }

  // Enlarge input buffer if necessary.
  if (!in_shared_memory_.get() ||
      in_buffer_size > in_shared_memory_->mapped_size()) {
    // Reserve 2x space to avoid frequent reallocations for initial frames.
    const size_t reserved_size = 2 * in_buffer_size;
    in_shared_memory_.reset(new base::SharedMemory);
    if (!in_shared_memory_->CreateAndMapAnonymous(reserved_size)) {
      base::AutoLock lock(lock_);
      decoder_status_ = FAILED;
      LOG(WARNING) << "CreateAndMapAnonymous failed, size=" << reserved_size;
      return;
    }
  }
  memcpy(in_shared_memory_->memory(), data, in_buffer_size);

  // No need to lock for |in_buffer_id_| since IsDecoding_Locked() is false.
  in_buffer_id_ = next_bitstream_buffer_id_;
  media::BitstreamBuffer in_buffer(in_buffer_id_, in_shared_memory_->handle(),
                                   in_buffer_size);
  // Mask against 30 bits, to avoid (undefined) wraparound on signed integer.
  next_bitstream_buffer_id_ = (next_bitstream_buffer_id_ + 1) & 0x3FFFFFFF;

  // The API of |decoder_| requires us to wrap the |out_buffer| in a VideoFrame.
  const gfx::Size dimensions = frame_format.frame_size;
  std::unique_ptr<media::VideoCaptureBufferHandle> out_buffer_access =
      out_buffer.handle_provider->GetHandleForInProcessAccess();
  base::SharedMemoryHandle out_handle =
      out_buffer.handle_provider->GetNonOwnedSharedMemoryHandleForLegacyIPC();
  scoped_refptr<media::VideoFrame> out_frame =
      media::VideoFrame::WrapExternalSharedMemory(
          media::PIXEL_FORMAT_I420,          // format
          dimensions,                        // coded_size
          gfx::Rect(dimensions),             // visible_rect
          dimensions,                        // natural_size
          out_buffer_access->data(),         // data
          out_buffer_access->mapped_size(),  // data_size
          out_handle,                        // handle
          0,                                 // shared_memory_offset
          timestamp);                        // timestamp
  if (!out_frame) {
    base::AutoLock lock(lock_);
    decoder_status_ = FAILED;
    LOG(ERROR) << "DecodeCapturedData: WrapExternalSharedMemory failed";
    return;
  }
  // Hold onto the buffer access handle for the lifetime of the VideoFrame, to
  // ensure the data pointers remain valid.
  out_frame->AddDestructionObserver(base::BindOnce(
      [](std::unique_ptr<media::VideoCaptureBufferHandle> handle) {},
      std::move(out_buffer_access)));
  out_frame->metadata()->SetDouble(media::VideoFrameMetadata::FRAME_RATE,
                                   frame_format.frame_rate);

  out_frame->metadata()->SetTimeTicks(media::VideoFrameMetadata::REFERENCE_TIME,
                                      reference_time);

  media::mojom::VideoFrameInfoPtr out_frame_info =
      media::mojom::VideoFrameInfo::New();
  out_frame_info->timestamp = timestamp;
  out_frame_info->pixel_format = media::PIXEL_FORMAT_I420;
  out_frame_info->coded_size = dimensions;
  out_frame_info->visible_rect = gfx::Rect(dimensions);
  out_frame_info->metadata = out_frame->metadata()->GetInternalValues().Clone();

  {
    base::AutoLock lock(lock_);
    decode_done_closure_ =
        base::Bind(decode_done_cb_, out_buffer.id, out_buffer.frame_feedback_id,
                   base::Passed(&out_buffer.access_permission),
                   base::Passed(&out_frame_info));
  }

  // base::Unretained is safe because |decoder_| is deleted on the IO thread.
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(&media::JpegDecodeAccelerator::Decode,
                                         base::Unretained(decoder_.get()),
                                         in_buffer, std::move(out_frame)));
}

void VideoCaptureGpuJpegDecoder::VideoFrameReady(int32_t bitstream_buffer_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  TRACE_EVENT0("jpeg", "VideoCaptureGpuJpegDecoder::VideoFrameReady");
  if (!has_received_decoded_frame_) {
    send_log_message_cb_.Run("Received decoded frame from Gpu Jpeg decoder");
    has_received_decoded_frame_ = true;
  }
  base::AutoLock lock(lock_);

  if (!IsDecoding_Locked()) {
    LOG(ERROR) << "Got decode response while not decoding";
    return;
  }

  if (bitstream_buffer_id != in_buffer_id_) {
    LOG(ERROR) << "Unexpected bitstream_buffer_id " << bitstream_buffer_id
               << ", expected " << in_buffer_id_;
    return;
  }
  in_buffer_id_ = media::JpegDecodeAccelerator::kInvalidBitstreamBufferId;

  decode_done_closure_.Run();
  decode_done_closure_.Reset();

  TRACE_EVENT_ASYNC_END0("jpeg", "VideoCaptureGpuJpegDecoder decoding",
                         bitstream_buffer_id);
}

void VideoCaptureGpuJpegDecoder::NotifyError(
    int32_t bitstream_buffer_id,
    media::JpegDecodeAccelerator::Error error) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  LOG(ERROR) << "Decode error, bitstream_buffer_id=" << bitstream_buffer_id
             << ", error=" << error;
  send_log_message_cb_.Run("Gpu Jpeg decoder failed");
  base::AutoLock lock(lock_);
  decode_done_closure_.Reset();
  decoder_status_ = FAILED;
}

// static
void VideoCaptureGpuJpegDecoder::RequestGPUInfoOnIOThread(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    base::WeakPtr<VideoCaptureGpuJpegDecoder> weak_this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  GpuProcessHost* host =
      GpuProcessHost::Get(GpuProcessHost::GPU_PROCESS_KIND_SANDBOXED, false);
  if (host) {
    host->RequestGPUInfo(
        base::Bind(&VideoCaptureGpuJpegDecoder::DidReceiveGPUInfoOnIOThread,
                   task_runner, weak_this));
  } else {
    DidReceiveGPUInfoOnIOThread(std::move(task_runner), std::move(weak_this),
                                gpu::GPUInfo());
  }
}

// static
void VideoCaptureGpuJpegDecoder::DidReceiveGPUInfoOnIOThread(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    base::WeakPtr<VideoCaptureGpuJpegDecoder> weak_this,
    const gpu::GPUInfo& gpu_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  media::mojom::JpegDecodeAcceleratorPtr remote_decoder;

  if (gpu_info.jpeg_decode_accelerator_supported) {
    GpuProcessHost* host =
        GpuProcessHost::Get(GpuProcessHost::GPU_PROCESS_KIND_SANDBOXED, false);
    if (host) {
      host->gpu_service()->CreateJpegDecodeAccelerator(
          mojo::MakeRequest(&remote_decoder));
    }
  }

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&VideoCaptureGpuJpegDecoder::FinishInitialization,
                     weak_this, remote_decoder.PassInterface()));
}

void VideoCaptureGpuJpegDecoder::FinishInitialization(
    media::mojom::JpegDecodeAcceleratorPtrInfo unbound_remote_decoder) {
  TRACE_EVENT0("gpu", "VideoCaptureGpuJpegDecoder::FinishInitialization");
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (unbound_remote_decoder.is_valid()) {
    base::AutoLock lock(lock_);
    decoder_ = std::make_unique<media::MojoJpegDecodeAccelerator>(
        BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
        std::move(unbound_remote_decoder));

    // base::Unretained is safe because |decoder_| is deleted on the IO thread.
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&media::JpegDecodeAccelerator::InitializeAsync,
                       base::Unretained(decoder_.get()), this,
                       media::BindToCurrentLoop(base::Bind(
                           &VideoCaptureGpuJpegDecoder::OnInitializationDone,
                           weak_ptr_factory_.GetWeakPtr()))));
  } else {
    OnInitializationDone(false);
  }
}

void VideoCaptureGpuJpegDecoder::OnInitializationDone(bool success) {
  TRACE_EVENT0("gpu", "VideoCaptureGpuJpegDecoder::OnInitializationDone");
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::AutoLock lock(lock_);
  if (!success) {
    BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE, decoder_.release());
    DLOG(ERROR) << "Failed to initialize JPEG decoder";
  }

  decoder_status_ = success ? INIT_PASSED : FAILED;
  RecordInitDecodeUMA_Locked();
}

bool VideoCaptureGpuJpegDecoder::IsDecoding_Locked() const {
  lock_.AssertAcquired();
  return !decode_done_closure_.is_null();
}

void VideoCaptureGpuJpegDecoder::RecordInitDecodeUMA_Locked() {
  UMA_HISTOGRAM_BOOLEAN("Media.VideoCaptureGpuJpegDecoder.InitDecodeSuccess",
                        decoder_status_ == INIT_PASSED);
}

void VideoCaptureGpuJpegDecoder::DestroyDecoderOnIOThread(
    base::WaitableEvent* event) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  decoder_.reset();
  event->Signal();
}

}  // namespace content
