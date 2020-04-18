// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/library_cdm/clear_key_cdm/ffmpeg_cdm_video_decoder.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "media/base/limits.h"
#include "media/cdm/library_cdm/cdm_host_proxy.h"
#include "media/ffmpeg/ffmpeg_common.h"
#include "media/ffmpeg/ffmpeg_decoding_loop.h"

namespace media {

static const int kDecodeThreads = 1;

static cdm::VideoFormat AVPixelFormatToCdmVideoFormat(
    AVPixelFormat pixel_format) {
  switch (pixel_format) {
    case AV_PIX_FMT_YUV420P:
      return cdm::kYv12;
    default:
      DVLOG(1) << "Unsupported AVPixelFormat: " << pixel_format;
  }
  return cdm::kUnknownVideoFormat;
}

static AVPixelFormat CdmVideoFormatToAVPixelFormat(
    cdm::VideoFormat video_format) {
  switch (video_format) {
    case cdm::kYv12:
    case cdm::kI420:
      return AV_PIX_FMT_YUV420P;
    case cdm::kUnknownVideoFormat:
    default:
      DVLOG(1) << "Unsupported cdm::VideoFormat: " << video_format;
  }
  return AV_PIX_FMT_NONE;
}

static AVCodecID CdmVideoCodecToCodecID(cdm::VideoCodec video_codec) {
  switch (video_codec) {
    case cdm::kCodecVp8:
      return AV_CODEC_ID_VP8;
    case cdm::kCodecH264:
      return AV_CODEC_ID_H264;
    case cdm::kCodecVp9:
      return AV_CODEC_ID_VP9;
    case cdm::kUnknownVideoCodec:
    default:
      NOTREACHED() << "Unsupported cdm::VideoCodec: " << video_codec;
      return AV_CODEC_ID_NONE;
  }
}

static int CdmVideoCodecProfileToProfileID(cdm::VideoCodecProfile profile) {
  switch (profile) {
    case cdm::kProfileNotNeeded:
      // For codecs that do not need a profile (e.g. VP8/VP9), does not define
      // an FFmpeg profile.
      return FF_PROFILE_UNKNOWN;
    case cdm::kH264ProfileBaseline:
      return FF_PROFILE_H264_BASELINE;
    case cdm::kH264ProfileMain:
      return FF_PROFILE_H264_MAIN;
    case cdm::kH264ProfileExtended:
      return FF_PROFILE_H264_EXTENDED;
    case cdm::kH264ProfileHigh:
      return FF_PROFILE_H264_HIGH;
    case cdm::kH264ProfileHigh10:
      return FF_PROFILE_H264_HIGH_10;
    case cdm::kH264ProfileHigh422:
      return FF_PROFILE_H264_HIGH_422;
    case cdm::kH264ProfileHigh444Predictive:
      return FF_PROFILE_H264_HIGH_444_PREDICTIVE;
    case cdm::kUnknownVideoCodecProfile:
    default:
      NOTREACHED() << "Unknown cdm::VideoCodecProfile: " << profile;
      return FF_PROFILE_UNKNOWN;
  }
}

static void CdmVideoDecoderConfigToAVCodecContext(
    const cdm::VideoDecoderConfig_2& config,
    AVCodecContext* codec_context) {
  codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_context->codec_id = CdmVideoCodecToCodecID(config.codec);
  codec_context->profile = CdmVideoCodecProfileToProfileID(config.profile);
  codec_context->coded_width = config.coded_size.width;
  codec_context->coded_height = config.coded_size.height;
  codec_context->pix_fmt = CdmVideoFormatToAVPixelFormat(config.format);

  if (config.extra_data) {
    codec_context->extradata_size = config.extra_data_size;
    codec_context->extradata = reinterpret_cast<uint8_t*>(
        av_malloc(config.extra_data_size + AV_INPUT_BUFFER_PADDING_SIZE));
    memcpy(codec_context->extradata, config.extra_data, config.extra_data_size);
    memset(codec_context->extradata + config.extra_data_size, 0,
           AV_INPUT_BUFFER_PADDING_SIZE);
  } else {
    codec_context->extradata = NULL;
    codec_context->extradata_size = 0;
  }
}

static void CopyPlane(const uint8_t* source,
                      int32_t source_stride,
                      int32_t target_stride,
                      int32_t rows,
                      int32_t copy_bytes_per_row,
                      uint8_t* target) {
  DCHECK(source);
  DCHECK(target);
  DCHECK_LE(copy_bytes_per_row, source_stride);
  DCHECK_LE(copy_bytes_per_row, target_stride);

  for (int i = 0; i < rows; ++i) {
    const int source_offset = i * source_stride;
    const int target_offset = i * target_stride;
    memcpy(target + target_offset, source + source_offset, copy_bytes_per_row);
  }
}

FFmpegCdmVideoDecoder::FFmpegCdmVideoDecoder(CdmHostProxy* cdm_host_proxy)
    : cdm_host_proxy_(cdm_host_proxy) {}

FFmpegCdmVideoDecoder::~FFmpegCdmVideoDecoder() {
  ReleaseFFmpegResources();
}

bool FFmpegCdmVideoDecoder::Initialize(
    const cdm::VideoDecoderConfig_2& config) {
  DVLOG(1) << "Initialize()";

  if (!IsValidOutputConfig(config.format, config.coded_size)) {
    LOG(ERROR) << "Initialize(): invalid video decoder configuration.";
    return false;
  }

  if (is_initialized_) {
    LOG(ERROR) << "Initialize(): Already initialized.";
    return false;
  }

  // Initialize AVCodecContext structure.
  codec_context_.reset(avcodec_alloc_context3(NULL));
  CdmVideoDecoderConfigToAVCodecContext(config, codec_context_.get());

  // Enable motion vector search (potentially slow), strong deblocking filter
  // for damaged macroblocks, and set our error detection sensitivity.
  codec_context_->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  codec_context_->err_recognition = AV_EF_CAREFUL;
  codec_context_->thread_count = kDecodeThreads;
  codec_context_->opaque = this;

  AVCodec* codec = avcodec_find_decoder(codec_context_->codec_id);
  if (!codec) {
    LOG(ERROR) << "Initialize(): avcodec_find_decoder failed.";
    return false;
  }

  int status;
  if ((status = avcodec_open2(codec_context_.get(), codec, NULL)) < 0) {
    LOG(ERROR) << "Initialize(): avcodec_open2 failed: " << status;
    return false;
  }

  decoding_loop_.reset(new FFmpegDecodingLoop(codec_context_.get()));
  is_initialized_ = true;

  return true;
}

void FFmpegCdmVideoDecoder::Deinitialize() {
  DVLOG(1) << "Deinitialize()";
  ReleaseFFmpegResources();
  is_initialized_ = false;
}

void FFmpegCdmVideoDecoder::Reset() {
  DVLOG(1) << "Reset()";
  avcodec_flush_buffers(codec_context_.get());
}

// static
bool FFmpegCdmVideoDecoder::IsValidOutputConfig(cdm::VideoFormat format,
                                                const cdm::Size& data_size) {
  return ((format == cdm::kYv12 || format == cdm::kI420) &&
          (data_size.width % 2) == 0 && (data_size.height % 2) == 0 &&
          data_size.width > 0 && data_size.height > 0 &&
          data_size.width <= limits::kMaxDimension &&
          data_size.height <= limits::kMaxDimension &&
          data_size.width * data_size.height <= limits::kMaxCanvas);
}

cdm::Status FFmpegCdmVideoDecoder::DecodeFrame(const uint8_t* compressed_frame,
                                               int32_t compressed_frame_size,
                                               int64_t timestamp,
                                               cdm::VideoFrame* decoded_frame) {
  DVLOG(1) << "DecodeFrame()";
  DCHECK(decoded_frame);

  // Create a packet for input data.
  AVPacket packet;
  av_init_packet(&packet);

  // The FFmpeg API does not allow us to have const read-only pointers.
  packet.data = const_cast<uint8_t*>(compressed_frame);
  packet.size = compressed_frame_size;

  // Let FFmpeg handle presentation timestamp reordering.
  codec_context_->reordered_opaque = timestamp;

  if (decoding_loop_->DecodePacket(
          &packet, base::BindRepeating(&FFmpegCdmVideoDecoder::OnNewFrame,
                                       base::Unretained(this))) !=
      FFmpegDecodingLoop::DecodeStatus::kOkay) {
    return cdm::kDecodeError;
  }

  if (pending_frames_.empty())
    return cdm::kNeedMoreData;

  auto frame = std::move(pending_frames_.front());
  pending_frames_.pop_front();

  if (!CopyAvFrameTo(frame.get(), decoded_frame)) {
    LOG(ERROR) << "DecodeFrame() could not copy video frame to output buffer.";
    return cdm::kDecodeError;
  }

  // If no frame was produced then signal that more data is required to produce
  // more frames.
  return cdm::kSuccess;
}

bool FFmpegCdmVideoDecoder::is_initialized() const {
  return is_initialized_;
}

bool FFmpegCdmVideoDecoder::OnNewFrame(AVFrame* frame) {
  // The decoder is in a bad state and not decoding correctly.
  // Checking for NULL avoids a crash.
  if (!frame->data[cdm::VideoFrame::kYPlane] ||
      !frame->data[cdm::VideoFrame::kUPlane] ||
      !frame->data[cdm::VideoFrame::kVPlane]) {
    LOG(ERROR) << "DecodeFrame(): Video frame has invalid frame data.";
    return false;
  }

  pending_frames_.emplace_back(av_frame_clone(frame));
  return true;
}

bool FFmpegCdmVideoDecoder::CopyAvFrameTo(AVFrame* frame,
                                          cdm::VideoFrame* cdm_video_frame) {
  DCHECK(cdm_video_frame);
  DCHECK_EQ(frame->format, AV_PIX_FMT_YUV420P);
  DCHECK_EQ(frame->width % 2, 0);
  DCHECK_EQ(frame->height % 2, 0);

  const int y_size = frame->width * frame->height;
  const int uv_size = y_size / 2;
  const int space_required = y_size + (uv_size * 2);

  DCHECK(!cdm_video_frame->FrameBuffer());
  cdm_video_frame->SetFrameBuffer(cdm_host_proxy_->Allocate(space_required));
  if (!cdm_video_frame->FrameBuffer()) {
    LOG(ERROR) << "CopyAvFrameTo() ClearKeyCdmHost::Allocate failed.";
    return false;
  }
  cdm_video_frame->FrameBuffer()->SetSize(space_required);

  CopyPlane(frame->data[cdm::VideoFrame::kYPlane],
            frame->linesize[cdm::VideoFrame::kYPlane], frame->width,
            frame->height, frame->width,
            cdm_video_frame->FrameBuffer()->Data());

  const int uv_stride = frame->width / 2;
  const int uv_rows = frame->height / 2;
  CopyPlane(frame->data[cdm::VideoFrame::kUPlane],
            frame->linesize[cdm::VideoFrame::kUPlane], uv_stride, uv_rows,
            uv_stride, cdm_video_frame->FrameBuffer()->Data() + y_size);

  CopyPlane(frame->data[cdm::VideoFrame::kVPlane],
            frame->linesize[cdm::VideoFrame::kVPlane], uv_stride, uv_rows,
            uv_stride,
            cdm_video_frame->FrameBuffer()->Data() + y_size + uv_size);

  AVPixelFormat format = static_cast<AVPixelFormat>(frame->format);
  cdm_video_frame->SetFormat(AVPixelFormatToCdmVideoFormat(format));

  cdm_video_frame->SetSize({frame->width, frame->height});

  cdm_video_frame->SetPlaneOffset(cdm::VideoFrame::kYPlane, 0);
  cdm_video_frame->SetPlaneOffset(cdm::VideoFrame::kUPlane, y_size);
  cdm_video_frame->SetPlaneOffset(cdm::VideoFrame::kVPlane, y_size + uv_size);

  cdm_video_frame->SetStride(cdm::VideoFrame::kYPlane, frame->width);
  cdm_video_frame->SetStride(cdm::VideoFrame::kUPlane, uv_stride);
  cdm_video_frame->SetStride(cdm::VideoFrame::kVPlane, uv_stride);

  cdm_video_frame->SetTimestamp(frame->reordered_opaque);

  return true;
}

void FFmpegCdmVideoDecoder::ReleaseFFmpegResources() {
  DVLOG(1) << "ReleaseFFmpegResources()";

  decoding_loop_.reset();
  codec_context_.reset();
}

}  // namespace media
