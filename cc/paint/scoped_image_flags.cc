// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/scoped_image_flags.h"

#include "cc/paint/image_provider.h"
#include "cc/paint/paint_image_builder.h"

namespace cc {
namespace {
SkIRect RoundOutRect(const SkRect& rect) {
  SkIRect result;
  rect.roundOut(&result);
  return result;
}
}  // namespace

ScopedImageFlags::DecodeStashingImageProvider::DecodeStashingImageProvider(
    ImageProvider* source_provider)
    : source_provider_(source_provider) {}
ScopedImageFlags::DecodeStashingImageProvider::~DecodeStashingImageProvider() =
    default;

ImageProvider::ScopedDecodedDrawImage
ScopedImageFlags::DecodeStashingImageProvider::GetDecodedDrawImage(
    const DrawImage& draw_image) {
  auto decode = source_provider_->GetDecodedDrawImage(draw_image);
  if (!decode)
    return ScopedDecodedDrawImage();

  // No need to add any destruction callback to the returned image. The images
  // decoded here match the lifetime of this provider.
  auto image_to_return = ScopedDecodedDrawImage(decode.decoded_image());
  decoded_images_.push_back(std::move(decode));
  return image_to_return;
}

ScopedImageFlags::ScopedImageFlags(ImageProvider* image_provider,
                                   const PaintFlags& flags,
                                   const SkMatrix& ctm)
    : decode_stashing_image_provider_(image_provider) {
  if (flags.getShader()->shader_type() == PaintShader::Type::kImage) {
    DecodeImageShader(flags, ctm);
  } else {
    DCHECK_EQ(flags.getShader()->shader_type(),
              PaintShader::Type::kPaintRecord);
    DecodeRecordShader(flags, ctm);
  }
}

ScopedImageFlags::~ScopedImageFlags() = default;

void ScopedImageFlags::DecodeImageShader(const PaintFlags& flags,
                                         const SkMatrix& ctm) {
  const PaintImage& paint_image = flags.getShader()->paint_image();
  SkMatrix matrix = flags.getShader()->GetLocalMatrix();

  SkMatrix total_image_matrix = matrix;
  total_image_matrix.preConcat(ctm);
  SkRect src_rect = SkRect::MakeIWH(paint_image.width(), paint_image.height());
  DrawImage draw_image(paint_image, RoundOutRect(src_rect),
                       flags.getFilterQuality(), total_image_matrix);
  auto decoded_draw_image =
      decode_stashing_image_provider_.GetDecodedDrawImage(draw_image);

  if (!decoded_draw_image)
    return;

  const auto& decoded_image = decoded_draw_image.decoded_image();
  DCHECK(decoded_image.image());

  bool need_scale = !decoded_image.is_scale_adjustment_identity();
  if (need_scale) {
    matrix.preScale(1.f / decoded_image.scale_adjustment().width(),
                    1.f / decoded_image.scale_adjustment().height());
  }

  sk_sp<SkImage> sk_image =
      sk_ref_sp<SkImage>(const_cast<SkImage*>(decoded_image.image().get()));
  PaintImage decoded_paint_image = PaintImageBuilder::WithDefault()
                                       .set_id(paint_image.stable_id())
                                       .set_image(std::move(sk_image))
                                       .TakePaintImage();
  decoded_flags_.emplace(flags);
  decoded_flags_.value().setFilterQuality(decoded_image.filter_quality());
  decoded_flags_.value().setShader(
      PaintShader::MakeImage(decoded_paint_image, flags.getShader()->tx(),
                             flags.getShader()->ty(), &matrix));
}

void ScopedImageFlags::DecodeRecordShader(const PaintFlags& flags,
                                          const SkMatrix& ctm) {
  auto decoded_shader = flags.getShader()->CreateDecodedPaintRecord(
      ctm, &decode_stashing_image_provider_);
  if (!decoded_shader)
    return;

  decoded_flags_.emplace(flags);
  decoded_flags_.value().setShader(std::move(decoded_shader));
}

}  // namespace cc
