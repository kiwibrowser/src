// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/library_cdm/clear_key_cdm/libvpx_cdm_video_decoder.h"

#include "base/logging.h"
#include "media/base/limits.h"
#include "media/cdm/library_cdm/cdm_host_proxy.h"
#include "third_party/libvpx/source/libvpx/vpx/vp8dx.h"
#include "third_party/libvpx/source/libvpx/vpx/vpx_decoder.h"

// Enable USE_COPYPLANE_WITH_LIBVPX to use |CopyPlane()| instead of memcpy to
// copy video frame data.
// #define USE_COPYPLANE_WITH_LIBVPX 1

namespace media {

static const int kDecodeThreads = 2;

LibvpxCdmVideoDecoder::LibvpxCdmVideoDecoder(CdmHostProxy* cdm_host_proxy)
    : cdm_host_proxy_(cdm_host_proxy) {}

LibvpxCdmVideoDecoder::~LibvpxCdmVideoDecoder() {
  Deinitialize();
}

bool LibvpxCdmVideoDecoder::Initialize(
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

  vpx_codec_ = new vpx_codec_ctx();
  vpx_codec_dec_cfg_t vpx_config = {0};
  vpx_config.w = config.coded_size.width;
  vpx_config.h = config.coded_size.height;
  vpx_config.threads = kDecodeThreads;

  vpx_codec_err_t status = vpx_codec_dec_init(
      vpx_codec_,
      config.codec == cdm::kCodecVp9 ? vpx_codec_vp9_dx() : vpx_codec_vp8_dx(),
      &vpx_config, 0);
  if (status != VPX_CODEC_OK) {
    LOG(ERROR) << "InitializeLibvpx(): vpx_codec_dec_init failed, ret="
               << status;
    delete vpx_codec_;
    vpx_codec_ = NULL;
  }

  is_initialized_ = true;
  return true;
}

void LibvpxCdmVideoDecoder::Deinitialize() {
  DVLOG(1) << "Deinitialize()";

  if (vpx_codec_) {
    vpx_codec_destroy(vpx_codec_);
    vpx_codec_ = NULL;
  }

  is_initialized_ = false;
}

void LibvpxCdmVideoDecoder::Reset() {
  DVLOG(1) << "Reset()";
}

// static
bool LibvpxCdmVideoDecoder::IsValidOutputConfig(cdm::VideoFormat format,
                                                const cdm::Size& data_size) {
  return ((format == cdm::kYv12 || format == cdm::kI420) &&
          (data_size.width % 2) == 0 && (data_size.height % 2) == 0 &&
          data_size.width > 0 && data_size.height > 0 &&
          data_size.width <= limits::kMaxDimension &&
          data_size.height <= limits::kMaxDimension &&
          data_size.width * data_size.height <= limits::kMaxCanvas);
}

cdm::Status LibvpxCdmVideoDecoder::DecodeFrame(const uint8_t* compressed_frame,
                                               int32_t compressed_frame_size,
                                               int64_t timestamp,
                                               cdm::VideoFrame* decoded_frame) {
  DVLOG(3) << __func__ << ": frame size = " << compressed_frame_size;
  DCHECK(decoded_frame);

  // Pass |compressed_frame| to libvpx.
  void* user_priv = reinterpret_cast<void*>(&timestamp);
  vpx_codec_err_t status = vpx_codec_decode(
      vpx_codec_, compressed_frame, compressed_frame_size, user_priv, 0);
  if (status != VPX_CODEC_OK) {
    LOG(ERROR) << "DecodeFrameLibvpx(): vpx_codec_decode failed, status="
               << status;
    return cdm::kDecodeError;
  }

  // Gets pointer to decoded data.
  vpx_codec_iter_t iter = NULL;
  vpx_image_ = vpx_codec_get_frame(vpx_codec_, &iter);
  if (!vpx_image_)
    return cdm::kNeedMoreData;

  if (vpx_image_->user_priv != reinterpret_cast<void*>(&timestamp)) {
    LOG(ERROR) << "DecodeFrameLibvpx() invalid output timestamp.";
    return cdm::kDecodeError;
  }
  decoded_frame->SetTimestamp(timestamp);

  if (!CopyVpxImageTo(decoded_frame)) {
    LOG(ERROR) << "DecodeFrameLibvpx() could not copy vpx image to output "
               << "buffer.";
    return cdm::kDecodeError;
  }

  return cdm::kSuccess;
}

bool LibvpxCdmVideoDecoder::is_initialized() const {
  return is_initialized_;
}

bool LibvpxCdmVideoDecoder::CopyVpxImageTo(cdm::VideoFrame* cdm_video_frame) {
  DCHECK(cdm_video_frame);
  DCHECK_EQ(vpx_image_->fmt, VPX_IMG_FMT_I420);
  DCHECK_EQ(vpx_image_->d_w % 2, 0U);
  DCHECK_EQ(vpx_image_->d_h % 2, 0U);

  const int y_size = vpx_image_->stride[VPX_PLANE_Y] * vpx_image_->d_h;
  const int uv_rows = vpx_image_->d_h / 2;
  const int u_size = vpx_image_->stride[VPX_PLANE_U] * uv_rows;
  const int v_size = vpx_image_->stride[VPX_PLANE_V] * uv_rows;
  const int space_required = y_size + u_size + v_size;

  DCHECK(!cdm_video_frame->FrameBuffer());
  cdm_video_frame->SetFrameBuffer(cdm_host_proxy_->Allocate(space_required));
  if (!cdm_video_frame->FrameBuffer()) {
    LOG(ERROR) << "CopyVpxImageTo() CdmHost::Allocate failed.";
    return false;
  }
  cdm_video_frame->FrameBuffer()->SetSize(space_required);

  memcpy(cdm_video_frame->FrameBuffer()->Data(),
         vpx_image_->planes[VPX_PLANE_Y], y_size);
  memcpy(cdm_video_frame->FrameBuffer()->Data() + y_size,
         vpx_image_->planes[VPX_PLANE_U], u_size);
  memcpy(cdm_video_frame->FrameBuffer()->Data() + y_size + u_size,
         vpx_image_->planes[VPX_PLANE_V], v_size);

  cdm_video_frame->SetFormat(cdm::kYv12);

  cdm::Size video_frame_size;
  video_frame_size.width = vpx_image_->d_w;
  video_frame_size.height = vpx_image_->d_h;
  cdm_video_frame->SetSize(video_frame_size);

  cdm_video_frame->SetPlaneOffset(cdm::VideoFrame::kYPlane, 0);
  cdm_video_frame->SetPlaneOffset(cdm::VideoFrame::kUPlane, y_size);
  cdm_video_frame->SetPlaneOffset(cdm::VideoFrame::kVPlane, y_size + u_size);

  cdm_video_frame->SetStride(cdm::VideoFrame::kYPlane,
                             vpx_image_->stride[VPX_PLANE_Y]);
  cdm_video_frame->SetStride(cdm::VideoFrame::kUPlane,
                             vpx_image_->stride[VPX_PLANE_U]);
  cdm_video_frame->SetStride(cdm::VideoFrame::kVPlane,
                             vpx_image_->stride[VPX_PLANE_V]);

  return true;
}

}  // namespace media
