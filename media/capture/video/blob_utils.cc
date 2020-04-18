// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/blob_utils.h"

#include "media/base/video_frame.h"
#include "media/capture/video_capture_types.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/skia/include/core/SkImage.h"
#include "ui/gfx/codec/png_codec.h"

namespace media {

mojom::BlobPtr Blobify(const uint8_t* buffer,
                       const uint32_t bytesused,
                       const VideoCaptureFormat& capture_format) {
  DCHECK(buffer);
  DCHECK(bytesused);
  DCHECK(capture_format.IsValid());

  const VideoPixelFormat pixel_format = capture_format.pixel_format;
  if (pixel_format == VideoPixelFormat::PIXEL_FORMAT_MJPEG) {
    mojom::BlobPtr blob = mojom::Blob::New();
    blob->data.resize(bytesused);
    memcpy(blob->data.data(), buffer, bytesused);
    blob->mime_type = "image/jpeg";
    return blob;
  }

  uint32_t src_format;
  if (pixel_format == VideoPixelFormat::PIXEL_FORMAT_UYVY)
    src_format = libyuv::FOURCC_UYVY;
  else if (pixel_format == VideoPixelFormat::PIXEL_FORMAT_YUY2)
    src_format = libyuv::FOURCC_YUY2;
  else if (pixel_format == VideoPixelFormat::PIXEL_FORMAT_I420)
    src_format = libyuv::FOURCC_I420;
  else if (pixel_format == VideoPixelFormat::PIXEL_FORMAT_RGB24)
    src_format = libyuv::FOURCC_24BG;
  else
    return nullptr;

  const gfx::Size frame_size = capture_format.frame_size;
  // PNGCodec does not support YUV formats, convert to a temporary ARGB buffer.
  std::unique_ptr<uint8_t[]> tmp_argb(
      new uint8_t[VideoFrame::AllocationSize(PIXEL_FORMAT_ARGB, frame_size)]);
  if (ConvertToARGB(buffer, bytesused, tmp_argb.get(), frame_size.width() * 4,
                    0 /* crop_x_pos */, 0 /* crop_y_pos */, frame_size.width(),
                    frame_size.height(), frame_size.width(),
                    frame_size.height(), libyuv::RotationMode::kRotate0,
                    src_format) != 0) {
    return nullptr;
  }

  mojom::BlobPtr blob = mojom::Blob::New();
  const gfx::PNGCodec::ColorFormat codec_color_format =
      (kN32_SkColorType == kRGBA_8888_SkColorType) ? gfx::PNGCodec::FORMAT_RGBA
                                                   : gfx::PNGCodec::FORMAT_BGRA;
  const bool result = gfx::PNGCodec::Encode(
      tmp_argb.get(), codec_color_format, frame_size, frame_size.width() * 4,
      true /* discard_transparency */, std::vector<gfx::PNGCodec::Comment>(),
      &blob->data);
  DCHECK(result);

  blob->mime_type = "image/png";
  return blob;
}

}  // namespace media
