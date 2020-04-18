// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/vaapi_jpeg_decode_accelerator.h"

#include <stddef.h>
#include <string.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "media/base/video_frame.h"
#include "media/filters/jpeg_parser.h"
#include "media/gpu/shared_memory_region.h"
#include "media/gpu/vaapi/vaapi_picture.h"
#include "third_party/libyuv/include/libyuv.h"

#define VLOGF(level) VLOG(level) << __func__ << "(): "
#define DVLOGF(level) DVLOG(level) << __func__ << "(): "

namespace media {

namespace {
// UMA errors that the VaapiJpegDecodeAccelerator class reports.
enum VAJDADecoderFailure {
  VAAPI_ERROR = 0,
  VAJDA_DECODER_FAILURES_MAX,
};

static void ReportToUMA(VAJDADecoderFailure failure) {
  UMA_HISTOGRAM_ENUMERATION("Media.VAJDA.DecoderFailure", failure,
                            VAJDA_DECODER_FAILURES_MAX + 1);
}

static unsigned int VaSurfaceFormatForJpeg(
    const JpegFrameHeader& frame_header) {
  // The range of sampling factor is [1, 4]. Pack them into integer to make the
  // matching code simpler. For example, 0x211 means the sampling factor are 2,
  // 1, 1 for 3 components.
  unsigned int h = 0, v = 0;
  for (int i = 0; i < frame_header.num_components; i++) {
    DCHECK_LE(frame_header.components[i].horizontal_sampling_factor, 4);
    DCHECK_LE(frame_header.components[i].vertical_sampling_factor, 4);
    h = h << 4 | frame_header.components[i].horizontal_sampling_factor;
    v = v << 4 | frame_header.components[i].vertical_sampling_factor;
  }

  switch (frame_header.num_components) {
    case 1:  // Grey image
      return VA_RT_FORMAT_YUV400;

    case 3:  // Y Cb Cr color image
      // See https://en.wikipedia.org/wiki/Chroma_subsampling for the
      // definition of these numbers.
      if (h == 0x211 && v == 0x211)
        return VA_RT_FORMAT_YUV420;

      if (h == 0x211 && v == 0x111)
        return VA_RT_FORMAT_YUV422;

      if (h == 0x111 && v == 0x111)
        return VA_RT_FORMAT_YUV444;

      if (h == 0x411 && v == 0x111)
        return VA_RT_FORMAT_YUV411;
  }
  VLOGF(1) << "Unsupported sampling factor: num_components="
           << frame_header.num_components << ", h=" << std::hex << h
           << ", v=" << v;

  return 0;
}

}  // namespace

VaapiJpegDecodeAccelerator::DecodeRequest::DecodeRequest(
    int32_t bitstream_buffer_id,
    std::unique_ptr<SharedMemoryRegion> shm,
    const scoped_refptr<VideoFrame>& video_frame)
    : bitstream_buffer_id(bitstream_buffer_id),
      shm(std::move(shm)),
      video_frame(video_frame) {}

VaapiJpegDecodeAccelerator::DecodeRequest::~DecodeRequest() {}

void VaapiJpegDecodeAccelerator::NotifyError(int32_t bitstream_buffer_id,
                                             Error error) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  VLOGF(1) << "Notifying of error " << error;
  DCHECK(client_);
  client_->NotifyError(bitstream_buffer_id, error);
}

void VaapiJpegDecodeAccelerator::NotifyErrorFromDecoderThread(
    int32_t bitstream_buffer_id,
    Error error) {
  DCHECK(decoder_task_runner_->BelongsToCurrentThread());
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&VaapiJpegDecodeAccelerator::NotifyError,
                                    weak_this_, bitstream_buffer_id, error));
}

void VaapiJpegDecodeAccelerator::VideoFrameReady(int32_t bitstream_buffer_id) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  client_->VideoFrameReady(bitstream_buffer_id);
}

VaapiJpegDecodeAccelerator::VaapiJpegDecodeAccelerator(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      io_task_runner_(io_task_runner),
      decoder_thread_("VaapiJpegDecoderThread"),
      va_surface_id_(VA_INVALID_SURFACE),
      weak_this_factory_(this) {
  weak_this_ = weak_this_factory_.GetWeakPtr();
}

VaapiJpegDecodeAccelerator::~VaapiJpegDecodeAccelerator() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  VLOGF(2) << "Destroying VaapiJpegDecodeAccelerator";

  weak_this_factory_.InvalidateWeakPtrs();
  decoder_thread_.Stop();
}

bool VaapiJpegDecodeAccelerator::Initialize(Client* client) {
  VLOGF(2);
  DCHECK(task_runner_->BelongsToCurrentThread());

  client_ = client;

  vaapi_wrapper_ =
      VaapiWrapper::Create(VaapiWrapper::kDecode, VAProfileJPEGBaseline,
                           base::Bind(&ReportToUMA, VAAPI_ERROR));

  if (!vaapi_wrapper_.get()) {
    VLOGF(1) << "Failed initializing VAAPI";
    return false;
  }

  if (!decoder_thread_.Start()) {
    VLOGF(1) << "Failed to start decoding thread.";
    return false;
  }
  decoder_task_runner_ = decoder_thread_.task_runner();

  return true;
}

bool VaapiJpegDecodeAccelerator::OutputPicture(
    VASurfaceID va_surface_id,
    int32_t input_buffer_id,
    const scoped_refptr<VideoFrame>& video_frame) {
  DCHECK(decoder_task_runner_->BelongsToCurrentThread());

  TRACE_EVENT1("jpeg", "VaapiJpegDecodeAccelerator::OutputPicture",
               "input_buffer_id", input_buffer_id);

  DVLOGF(4) << "Outputting VASurface " << va_surface_id
            << " into video_frame associated with input buffer id "
            << input_buffer_id;

  VAImage image;
  VAImageFormat format;
  const uint32_t kI420Fourcc = VA_FOURCC('I', '4', '2', '0');
  memset(&image, 0, sizeof(image));
  memset(&format, 0, sizeof(format));
  format.fourcc = kI420Fourcc;
  format.byte_order = VA_LSB_FIRST;
  format.bits_per_pixel = 12;  // 12 for I420

  uint8_t* mem = nullptr;
  gfx::Size coded_size = video_frame->coded_size();
  if (!vaapi_wrapper_->GetVaImage(va_surface_id, &format, coded_size, &image,
                                  reinterpret_cast<void**>(&mem))) {
    VLOGF(1) << "Cannot get VAImage";
    return false;
  }

  // Copy image content from VAImage to VideoFrame.
  // The component order of VAImage I420 are Y, U, and V.
  DCHECK_EQ(image.num_planes, 3u);
  DCHECK_GE(image.width, coded_size.width());
  DCHECK_GE(image.height, coded_size.height());
  const uint8_t* src_y = mem + image.offsets[0];
  const uint8_t* src_u = mem + image.offsets[1];
  const uint8_t* src_v = mem + image.offsets[2];
  size_t src_y_stride = image.pitches[0];
  size_t src_u_stride = image.pitches[1];
  size_t src_v_stride = image.pitches[2];
  uint8_t* dst_y = video_frame->data(VideoFrame::kYPlane);
  uint8_t* dst_u = video_frame->data(VideoFrame::kUPlane);
  uint8_t* dst_v = video_frame->data(VideoFrame::kVPlane);
  size_t dst_y_stride = video_frame->stride(VideoFrame::kYPlane);
  size_t dst_u_stride = video_frame->stride(VideoFrame::kUPlane);
  size_t dst_v_stride = video_frame->stride(VideoFrame::kVPlane);

  if (libyuv::I420Copy(src_y, src_y_stride,  // Y
                       src_u, src_u_stride,  // U
                       src_v, src_v_stride,  // V
                       dst_y, dst_y_stride,  // Y
                       dst_u, dst_u_stride,  // U
                       dst_v, dst_v_stride,  // V
                       coded_size.width(), coded_size.height())) {
    VLOGF(1) << "I420Copy failed";
    return false;
  }

  vaapi_wrapper_->ReturnVaImage(&image);

  task_runner_->PostTask(
      FROM_HERE, base::Bind(&VaapiJpegDecodeAccelerator::VideoFrameReady,
                            weak_this_, input_buffer_id));

  return true;
}

void VaapiJpegDecodeAccelerator::DecodeTask(
    const std::unique_ptr<DecodeRequest>& request) {
  DVLOGF(4);
  DCHECK(decoder_task_runner_->BelongsToCurrentThread());
  TRACE_EVENT0("jpeg", "DecodeTask");

  JpegParseResult parse_result;
  if (!ParseJpegPicture(
          reinterpret_cast<const uint8_t*>(request->shm->memory()),
          request->shm->size(), &parse_result)) {
    VLOGF(1) << "ParseJpegPicture failed";
    NotifyErrorFromDecoderThread(request->bitstream_buffer_id,
                                 PARSE_JPEG_FAILED);
    return;
  }

  unsigned int new_va_rt_format =
      VaSurfaceFormatForJpeg(parse_result.frame_header);
  if (!new_va_rt_format) {
    VLOGF(1) << "Unsupported subsampling";
    NotifyErrorFromDecoderThread(request->bitstream_buffer_id,
                                 UNSUPPORTED_JPEG);
    return;
  }

  // Reuse VASurface if size doesn't change.
  gfx::Size new_coded_size(parse_result.frame_header.coded_width,
                           parse_result.frame_header.coded_height);
  if (new_coded_size != coded_size_ || va_surface_id_ == VA_INVALID_SURFACE ||
      new_va_rt_format != va_rt_format_) {
    vaapi_wrapper_->DestroySurfaces();
    va_surface_id_ = VA_INVALID_SURFACE;
    va_rt_format_ = new_va_rt_format;

    std::vector<VASurfaceID> va_surfaces;
    if (!vaapi_wrapper_->CreateSurfaces(va_rt_format_, new_coded_size, 1,
                                        &va_surfaces)) {
      VLOGF(1) << "Create VA surface failed";
      NotifyErrorFromDecoderThread(request->bitstream_buffer_id,
                                   PLATFORM_FAILURE);
      return;
    }
    va_surface_id_ = va_surfaces[0];
    coded_size_ = new_coded_size;
  }

  if (!VaapiJpegDecoder::Decode(vaapi_wrapper_.get(), parse_result,
                                va_surface_id_)) {
    VLOGF(1) << "Decode JPEG failed";
    NotifyErrorFromDecoderThread(request->bitstream_buffer_id,
                                 PLATFORM_FAILURE);
    return;
  }

  if (!OutputPicture(va_surface_id_, request->bitstream_buffer_id,
                     request->video_frame)) {
    VLOGF(1) << "Output picture failed";
    NotifyErrorFromDecoderThread(request->bitstream_buffer_id,
                                 PLATFORM_FAILURE);
    return;
  }
}

void VaapiJpegDecodeAccelerator::Decode(
    const BitstreamBuffer& bitstream_buffer,
    const scoped_refptr<VideoFrame>& video_frame) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  TRACE_EVENT1("jpeg", "Decode", "input_id", bitstream_buffer.id());

  DVLOGF(4) << "Mapping new input buffer id: " << bitstream_buffer.id()
            << " size: " << bitstream_buffer.size();

  // SharedMemoryRegion will take over the |bitstream_buffer.handle()|.
  std::unique_ptr<SharedMemoryRegion> shm(
      new SharedMemoryRegion(bitstream_buffer, true));

  if (bitstream_buffer.id() < 0) {
    VLOGF(1) << "Invalid bitstream_buffer, id: " << bitstream_buffer.id();
    NotifyErrorFromDecoderThread(bitstream_buffer.id(), INVALID_ARGUMENT);
    return;
  }

  if (!shm->Map()) {
    VLOGF(1) << "Failed to map input buffer";
    NotifyErrorFromDecoderThread(bitstream_buffer.id(), UNREADABLE_INPUT);
    return;
  }

  std::unique_ptr<DecodeRequest> request(
      new DecodeRequest(bitstream_buffer.id(), std::move(shm), video_frame));

  decoder_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VaapiJpegDecodeAccelerator::DecodeTask,
                            base::Unretained(this), base::Passed(&request)));
}

bool VaapiJpegDecodeAccelerator::IsSupported() {
  return VaapiWrapper::IsJpegDecodeSupported();
}

}  // namespace media
