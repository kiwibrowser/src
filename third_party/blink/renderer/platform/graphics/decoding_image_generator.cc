/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/graphics/decoding_image_generator.h"

#include <utility>

#include <memory>
#include "third_party/blink/renderer/platform/graphics/image_frame_generator.h"
#include "third_party/blink/renderer/platform/image-decoders/image_decoder.h"
#include "third_party/blink/renderer/platform/image-decoders/segment_reader.h"
#include "third_party/blink/renderer/platform/instrumentation/platform_instrumentation.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/skia/include/core/SkData.h"

namespace blink {

// static
std::unique_ptr<SkImageGenerator>
DecodingImageGenerator::CreateAsSkImageGenerator(sk_sp<SkData> data) {
  scoped_refptr<SegmentReader> segment_reader =
      SegmentReader::CreateFromSkData(std::move(data));
  // We just need the size of the image, so we have to temporarily create an
  // ImageDecoder. Since we only need the size, the premul and gamma settings
  // don't really matter.
  std::unique_ptr<ImageDecoder> decoder = ImageDecoder::Create(
      segment_reader, true, ImageDecoder::kAlphaPremultiplied,
      ColorBehavior::TransformToSRGB());
  if (!decoder || !decoder->IsSizeAvailable())
    return nullptr;

  const IntSize size = decoder->Size();
  const SkImageInfo info =
      SkImageInfo::MakeN32(size.Width(), size.Height(), kPremul_SkAlphaType,
                           decoder->ColorSpaceForSkImages());

  scoped_refptr<ImageFrameGenerator> frame = ImageFrameGenerator::Create(
      SkISize::Make(size.Width(), size.Height()), false,
      decoder->GetColorBehavior(), decoder->GetSupportedDecodeSizes());
  if (!frame)
    return nullptr;

  std::vector<FrameMetadata> frames = {FrameMetadata()};
  sk_sp<DecodingImageGenerator> generator = DecodingImageGenerator::Create(
      std::move(frame), info, std::move(segment_reader), std::move(frames),
      PaintImage::GetNextContentId(), true);
  return std::make_unique<SkiaPaintImageGenerator>(
      std::move(generator), PaintImage::kDefaultFrameIndex);
}

// static
sk_sp<DecodingImageGenerator> DecodingImageGenerator::Create(
    scoped_refptr<ImageFrameGenerator> frame_generator,
    const SkImageInfo& info,
    scoped_refptr<SegmentReader> data,
    std::vector<FrameMetadata> frames,
    PaintImage::ContentId content_id,
    bool all_data_received) {
  return sk_sp<DecodingImageGenerator>(new DecodingImageGenerator(
      std::move(frame_generator), info, std::move(data), std::move(frames),
      content_id, all_data_received));
}

DecodingImageGenerator::DecodingImageGenerator(
    scoped_refptr<ImageFrameGenerator> frame_generator,
    const SkImageInfo& info,
    scoped_refptr<SegmentReader> data,
    std::vector<FrameMetadata> frames,
    PaintImage::ContentId complete_frame_content_id,
    bool all_data_received)
    : PaintImageGenerator(info, std::move(frames)),
      frame_generator_(std::move(frame_generator)),
      data_(std::move(data)),
      all_data_received_(all_data_received),
      can_yuv_decode_(false),
      complete_frame_content_id_(complete_frame_content_id) {}

DecodingImageGenerator::~DecodingImageGenerator() = default;

sk_sp<SkData> DecodingImageGenerator::GetEncodedData() const {
  TRACE_EVENT0("blink", "DecodingImageGenerator::refEncodedData");

  // getAsSkData() may require copying, but the clients of this function are
  // serializers, which want the data even if it requires copying, and even
  // if the data is incomplete. (Otherwise they would potentially need to
  // decode the partial image in order to re-encode it.)
  return data_->GetAsSkData();
}

bool DecodingImageGenerator::GetPixels(const SkImageInfo& dst_info,
                                       void* pixels,
                                       size_t row_bytes,
                                       size_t frame_index,
                                       uint32_t lazy_pixel_ref) {
  TRACE_EVENT1("blink", "DecodingImageGenerator::getPixels", "frame index",
               static_cast<int>(frame_index));

  // Implementation only supports decoding to a supported size.
  if (dst_info.dimensions() != GetSupportedDecodeSize(dst_info.dimensions())) {
    return false;
  }

  // TODO(vmpstr): We could do the color type conversion here by getting N32
  // colortype decode first, and then converting to whatever was requested.
  if (dst_info.colorType() != kN32_SkColorType) {
    return false;
  }

  // Skip the check for alphaType.  blink::ImageFrame may have changed the
  // owning SkBitmap to kOpaque_SkAlphaType after fully decoding the image
  // frame, so if we see a request for opaque, that is ok even if our initial
  // alpha type was not opaque.

  // Pass decodeColorSpace to the decoder.  That is what we can expect the
  // output to be.
  SkColorSpace* decode_color_space = GetSkImageInfo().colorSpace();
  SkImageInfo decode_info =
      dst_info.makeColorSpace(sk_ref_sp(decode_color_space));

  const bool needs_color_xform =
      decode_color_space && dst_info.colorSpace() &&
      !SkColorSpace::Equals(decode_color_space, dst_info.colorSpace());
  ImageDecoder::AlphaOption alpha_option = ImageDecoder::kAlphaPremultiplied;
  if (needs_color_xform && !decode_info.isOpaque()) {
    alpha_option = ImageDecoder::kAlphaNotPremultiplied;
    decode_info = decode_info.makeAlphaType(kUnpremul_SkAlphaType);
  }

  PlatformInstrumentation::WillDecodeLazyPixelRef(lazy_pixel_ref);
  const bool decoded = frame_generator_->DecodeAndScale(
      data_.get(), all_data_received_, frame_index, decode_info, pixels,
      row_bytes, alpha_option);
  PlatformInstrumentation::DidDecodeLazyPixelRef();

  if (decoded && needs_color_xform) {
    TRACE_EVENT0("blink", "DecodingImageGenerator::getPixels - apply xform");
    SkPixmap src(decode_info, pixels, row_bytes);

    // kIgnore ensures that we perform the premultiply (if necessary) in the dst
    // space.
    const bool converted = src.readPixels(dst_info, pixels, row_bytes, 0, 0,
                                          SkTransferFunctionBehavior::kIgnore);
    DCHECK(converted);
  }

  return decoded;
}

bool DecodingImageGenerator::QueryYUV8(SkYUVSizeInfo* size_info,
                                       SkYUVColorSpace* color_space) const {
  // YUV decoding does not currently support progressive decoding. See comment
  // in ImageFrameGenerator.h.
  if (!can_yuv_decode_ || !all_data_received_)
    return false;

  TRACE_EVENT0("blink", "DecodingImageGenerator::queryYUV8");

  if (color_space)
    *color_space = kJPEG_SkYUVColorSpace;

  return frame_generator_->GetYUVComponentSizes(data_.get(), size_info);
}

bool DecodingImageGenerator::GetYUV8Planes(const SkYUVSizeInfo& size_info,
                                           void* planes[3],
                                           size_t frame_index,
                                           uint32_t lazy_pixel_ref) {
  // YUV decoding does not currently support progressive decoding. See comment
  // in ImageFrameGenerator.h.
  DCHECK(can_yuv_decode_);
  DCHECK(all_data_received_);

  TRACE_EVENT0("blink", "DecodingImageGenerator::getYUV8Planes");

  PlatformInstrumentation::WillDecodeLazyPixelRef(lazy_pixel_ref);
  bool decoded =
      frame_generator_->DecodeToYUV(data_.get(), frame_index, size_info.fSizes,
                                    planes, size_info.fWidthBytes);
  PlatformInstrumentation::DidDecodeLazyPixelRef();

  return decoded;
}

SkISize DecodingImageGenerator::GetSupportedDecodeSize(
    const SkISize& requested_size) const {
  return frame_generator_->GetSupportedDecodeSize(requested_size);
}

PaintImage::ContentId DecodingImageGenerator::GetContentIdForFrame(
    size_t frame_index) const {
  DCHECK_LT(frame_index, GetFrameMetadata().size());

  // If we have all the data for the image, or this particular frame, we can
  // consider the decoded frame constant.
  if (all_data_received_ || GetFrameMetadata().at(frame_index).complete)
    return complete_frame_content_id_;

  return PaintImageGenerator::GetContentIdForFrame(frame_index);
}

}  // namespace blink
