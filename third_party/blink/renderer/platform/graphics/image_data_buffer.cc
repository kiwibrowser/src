/*
 * Copyright (c) 2008, Google Inc. All rights reserved.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/graphics/image_data_buffer.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/image-encoders/image_encoder.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/text/base64.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSwizzle.h"
#include "third_party/skia/include/encode/SkJpegEncoder.h"

namespace blink {

ImageDataBuffer::ImageDataBuffer(scoped_refptr<StaticBitmapImage> image) {
  if (!image)
    return;
  retained_image_ = image->PaintImageForCurrentFrame().GetSkImage();
  if (!retained_image_)
    return;
  if (retained_image_->isTextureBacked() ||
      retained_image_->isLazyGenerated() ||
      retained_image_->alphaType() != kUnpremul_SkAlphaType) {
    // Unpremul is handled upfront, using readPixels, which will correctly clamp
    // premul color values that would otherwise cause overflows in the skia
    // encoder unpremul logic.
    SkColorType colorType = retained_image_->colorType();
    if (colorType == kRGBA_8888_SkColorType ||
        colorType == kBGRA_8888_SkColorType)
      colorType = kN32_SkColorType;  // Work around for bug with JPEG encoder
    const SkImageInfo info =
        SkImageInfo::Make(retained_image_->width(), retained_image_->height(),
                          retained_image_->colorType(), kUnpremul_SkAlphaType,
                          retained_image_->refColorSpace());
    const size_t rowBytes = info.minRowBytes();
    size_t size = info.computeByteSize(rowBytes);
    if (SkImageInfo::ByteSizeOverflowed(size))
      return;

    sk_sp<SkData> data = SkData::MakeUninitialized(size);
    pixmap_ = {info, data->writable_data(), info.minRowBytes()};
    if (!retained_image_->readPixels(pixmap_, 0, 0)) {
      pixmap_.reset();
      return;
    }
    retained_image_ = SkImage::MakeRasterData(info, std::move(data), rowBytes);
  } else {
    if (!retained_image_->peekPixels(&pixmap_))
      return;
  }
  is_valid_ = true;
  size_ = IntSize(image->width(), image->height());
}

ImageDataBuffer::ImageDataBuffer(const SkPixmap& pixmap)
    : pixmap_(pixmap),
      size_(IntSize(pixmap.width(), pixmap.height())) {
  is_valid_ = pixmap_.addr() && !size_.IsEmpty();
}

std::unique_ptr<ImageDataBuffer> ImageDataBuffer::Create(
    scoped_refptr<StaticBitmapImage> image) {
  std::unique_ptr<ImageDataBuffer> buffer =
      base::WrapUnique(new ImageDataBuffer(image));
  if (!buffer->IsValid())
    return nullptr;
  return buffer;
}

std::unique_ptr<ImageDataBuffer> ImageDataBuffer::Create(
    const SkPixmap& pixmap) {
  std::unique_ptr<ImageDataBuffer> buffer =
      base::WrapUnique(new ImageDataBuffer(pixmap));
  if (!buffer->IsValid())
    return nullptr;
  return buffer;
}

const unsigned char* ImageDataBuffer::Pixels() const {
  DCHECK(is_valid_);
  return static_cast<const unsigned char*>(pixmap_.addr());
}

bool ImageDataBuffer::EncodeImage(const String& mime_type,
                                  const double& quality,
                                  Vector<unsigned char>* encoded_image) const {
  return EncodeImageInternal(mime_type, quality, encoded_image, pixmap_);
}

bool ImageDataBuffer::EncodeImageInternal(const String& mime_type,
                                          const double& quality,
                                          Vector<unsigned char>* encoded_image,
                                          const SkPixmap& pixmap) const {
  DCHECK(is_valid_);

  if (mime_type == "image/jpeg") {
    SkJpegEncoder::Options options;
    options.fQuality = ImageEncoder::ComputeJpegQuality(quality);
    options.fAlphaOption = SkJpegEncoder::AlphaOption::kBlendOnBlack;
    // When the gamma is linear (which is always the case with currently
    // supported color spaces in F16 format), it does not matter whether we use
    // kRespect or kIgnore, but the JPEG encoder does not support kIgnore with
    // F16 for some reason, so we switch to kRespect in that case, with no
    // consequence on the encoded output.
    options.fBlendBehavior = pixmap.colorType() == kRGBA_F16_SkColorType
                                 ? SkTransferFunctionBehavior::kRespect
                                 : SkTransferFunctionBehavior::kIgnore;
    if (options.fQuality == 100) {
      options.fDownsample = SkJpegEncoder::Downsample::k444;
    }
    return ImageEncoder::Encode(encoded_image, pixmap, options);
  }

  if (mime_type == "image/webp") {
    SkWebpEncoder::Options options = ImageEncoder::ComputeWebpOptions(
        quality, SkTransferFunctionBehavior::kIgnore);
    return ImageEncoder::Encode(encoded_image, pixmap, options);
  }

  DCHECK_EQ(mime_type, "image/png");
  SkPngEncoder::Options options;
  options.fFilterFlags = SkPngEncoder::FilterFlag::kSub;
  options.fZLibLevel = 3;
  options.fUnpremulBehavior = SkTransferFunctionBehavior::kIgnore;
  return ImageEncoder::Encode(encoded_image, pixmap, options);
}

String ImageDataBuffer::ToDataURL(const String& mime_type,
                                  const double& quality) const {
  DCHECK(is_valid_);
  DCHECK(MIMETypeRegistry::IsSupportedImageMIMETypeForEncoding(mime_type));

  // toDataURL always encodes in sRGB and does not include the color space
  // information.
  sk_sp<SkImage> skia_image = nullptr;
  SkPixmap pixmap = pixmap_;
  if (pixmap.colorSpace()) {
    if (!pixmap.colorSpace()->isSRGB()) {
      skia_image = SkImage::MakeFromRaster(pixmap, nullptr, nullptr);
      skia_image = skia_image->makeColorSpace(
          SkColorSpace::MakeSRGB(), SkTransferFunctionBehavior::kIgnore);
      skia_image->peekPixels(&pixmap);
    }
    pixmap.setColorSpace(nullptr);
  }

  Vector<unsigned char> result;
  if (!EncodeImageInternal(mime_type, quality, &result, pixmap))
    return "data:,";

  return "data:" + mime_type + ";base64," + Base64Encode(result);
}

}  // namespace blink
