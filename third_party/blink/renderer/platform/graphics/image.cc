/*
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/graphics/image.h"

#include "build/build_config.h"
#include "cc/tiles/software_image_decode_cache.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/float_size.h"
#include "third_party/blink/renderer/platform/graphics/bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/deferred_image_decoder.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_image.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_shader.h"
#include "third_party/blink/renderer/platform/graphics/scoped_interpolation_quality.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/platform_instrumentation.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/length.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/checked_numeric.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/skia/include/core/SkImage.h"

#include <math.h>
#include <tuple>

namespace blink {

Image::Image(ImageObserver* observer, bool is_multipart)
    : image_observer_disabled_(false),
      image_observer_(observer),
      stable_image_id_(PaintImage::GetNextId()),
      is_multipart_(is_multipart),
      high_contrast_classification_(
          HighContrastClassification::kNotClassified) {}

Image::~Image() = default;

Image* Image::NullImage() {
  DCHECK(IsMainThread());
  DEFINE_STATIC_REF(Image, null_image, (BitmapImage::Create()));
  return null_image;
}

// static
cc::ImageDecodeCache& Image::SharedCCDecodeCache() {
  // This denotes the allocated locked memory budget for the cache used for
  // book-keeping. The cache indicates when the total memory locked exceeds this
  // budget in cc::DecodedDrawImage.
  static const size_t kLockedMemoryLimitBytes = 64 * 1024 * 1024;
  DEFINE_THREAD_SAFE_STATIC_LOCAL(cc::SoftwareImageDecodeCache,
                                  image_decode_cache,
                                  (kN32_SkColorType, kLockedMemoryLimitBytes));
  return image_decode_cache;
}

scoped_refptr<Image> Image::LoadPlatformResource(const char* name) {
  const WebData& resource = Platform::Current()->GetDataResource(name);
  if (resource.IsEmpty())
    return Image::NullImage();

  scoped_refptr<Image> image = BitmapImage::Create();
  image->SetData(resource, true);
  return image;
}

bool Image::SupportsType(const String& type) {
  return MIMETypeRegistry::IsSupportedImageResourceMIMEType(type);
}

Image::SizeAvailability Image::SetData(scoped_refptr<SharedBuffer> data,
                                       bool all_data_received) {
  encoded_image_data_ = std::move(data);
  if (!encoded_image_data_.get())
    return kSizeAvailable;

  int length = encoded_image_data_->size();
  if (!length)
    return kSizeAvailable;

  return DataChanged(all_data_received);
}

String Image::FilenameExtension() const {
  return String();
}

void Image::DrawTiledBackground(GraphicsContext& ctxt,
                                const FloatRect& dest_rect,
                                const FloatPoint& src_point,
                                const FloatSize& scaled_tile_size,
                                SkBlendMode op,
                                const FloatSize& repeat_spacing) {
  if (scaled_tile_size.IsEmpty())
    return;

  FloatSize intrinsic_tile_size(Size());
  if (HasRelativeSize()) {
    intrinsic_tile_size.SetWidth(scaled_tile_size.Width());
    intrinsic_tile_size.SetHeight(scaled_tile_size.Height());
  }

  const FloatSize scale(
      scaled_tile_size.Width() / intrinsic_tile_size.Width(),
      scaled_tile_size.Height() / intrinsic_tile_size.Height());

  const FloatRect one_tile_rect = ComputeTileContaining(
      dest_rect.Location(), scaled_tile_size, src_point, repeat_spacing);

  // Check and see if a single draw of the image can cover the entire area we
  // are supposed to tile.
  if (one_tile_rect.Contains(dest_rect)) {
    const FloatRect visible_src_rect =
        ComputeSubsetForTile(one_tile_rect, dest_rect, intrinsic_tile_size);
    ctxt.DrawImage(this, kSyncDecode, dest_rect, &visible_src_rect, op,
                   kDoNotRespectImageOrientation);
    return;
  }

  FloatRect tile_rect(FloatPoint(), intrinsic_tile_size);
  DrawPattern(ctxt, tile_rect, scale, one_tile_rect.Location(), op, dest_rect,
              repeat_spacing);

  StartAnimation();
}

void Image::DrawTiledBorder(GraphicsContext& ctxt,
                            const FloatRect& dst_rect,
                            const FloatRect& src_rect,
                            const FloatSize& provided_tile_scale_factor,
                            TileRule h_rule,
                            TileRule v_rule,
                            SkBlendMode op) {
  // TODO(cavalcantii): see crbug.com/662513.
  FloatSize tile_scale_factor = provided_tile_scale_factor;
  if (v_rule == kRoundTile) {
    float v_repetitions = std::max(
        1.0f, roundf(dst_rect.Height() /
                     (tile_scale_factor.Height() * src_rect.Height())));
    tile_scale_factor.SetHeight(dst_rect.Height() /
                                (src_rect.Height() * v_repetitions));
  }

  if (h_rule == kRoundTile) {
    float h_repetitions =
        std::max(1.0f, roundf(dst_rect.Width() /
                              (tile_scale_factor.Width() * src_rect.Width())));
    tile_scale_factor.SetWidth(dst_rect.Width() /
                               (src_rect.Width() * h_repetitions));
  }

  // We want to construct the phase such that the pattern is centered (when
  // stretch is not set for a particular rule).
  float v_phase = tile_scale_factor.Height() * src_rect.Y();
  float h_phase = tile_scale_factor.Width() * src_rect.X();
  if (v_rule == kRepeatTile) {
    float scaled_tile_height = tile_scale_factor.Height() * src_rect.Height();
    v_phase -= (dst_rect.Height() - scaled_tile_height) / 2;
  }

  if (h_rule == kRepeatTile) {
    float scaled_tile_width = tile_scale_factor.Width() * src_rect.Width();
    h_phase -= (dst_rect.Width() - scaled_tile_width) / 2;
  }

  FloatSize spacing;
  auto calculate_space_needed =
      [](const float destination,
         const float source) -> std::tuple<bool, float> {
    DCHECK_GT(source, 0);
    DCHECK_GT(destination, 0);

    float repeat_tiles_count = floorf(destination / source);
    if (!repeat_tiles_count)
      return std::make_tuple(false, -1);

    float space = destination;
    space -= source * repeat_tiles_count;
    space /= repeat_tiles_count + 1.0;

    return std::make_tuple(true, space);
  };

  if (v_rule == kSpaceTile) {
    std::tuple<bool, float> space =
        calculate_space_needed(dst_rect.Height(), src_rect.Height());
    if (!std::get<0>(space))
      return;

    spacing.SetHeight(std::get<1>(space));
    tile_scale_factor.SetHeight(1.0);
    v_phase = src_rect.Y();
    v_phase -= spacing.Height();
  }

  if (h_rule == kSpaceTile) {
    std::tuple<bool, float> space =
        calculate_space_needed(dst_rect.Width(), src_rect.Width());
    if (!std::get<0>(space))
      return;

    spacing.SetWidth(std::get<1>(space));
    tile_scale_factor.SetWidth(1.0);
    h_phase = src_rect.X();
    h_phase -= spacing.Width();
  }

  FloatPoint pattern_phase(dst_rect.X() - h_phase, dst_rect.Y() - v_phase);

  // TODO(cavalcantii): see crbug.com/662507.
  if ((h_rule == kRoundTile) || (v_rule == kRoundTile)) {
    ScopedInterpolationQuality interpolation_quality_scope(ctxt,
                                                           kInterpolationLow);
    DrawPattern(ctxt, src_rect, tile_scale_factor, pattern_phase, op, dst_rect,
                FloatSize());
  } else {
    DrawPattern(ctxt, src_rect, tile_scale_factor, pattern_phase, op, dst_rect,
                spacing);
  }

  StartAnimation();
}

namespace {

sk_sp<PaintShader> CreatePatternShader(const PaintImage& image,
                                       const SkMatrix& shader_matrix,
                                       const PaintFlags& paint,
                                       const FloatSize& spacing,
                                       SkShader::TileMode tmx,
                                       SkShader::TileMode tmy) {
  if (spacing.IsZero()) {
    return PaintShader::MakeImage(image, tmx, tmy, &shader_matrix);
  }

  // Arbitrary tiling is currently only supported for SkPictureShader, so we use
  // that instead of a plain bitmap shader to implement spacing.
  const SkRect tile_rect = SkRect::MakeWH(image.width() + spacing.Width(),
                                          image.height() + spacing.Height());

  PaintRecorder recorder;
  PaintCanvas* canvas = recorder.beginRecording(tile_rect);
  canvas->drawImage(image, 0, 0, &paint);

  return PaintShader::MakePaintRecord(recorder.finishRecordingAsPicture(),
                                      tile_rect, tmx, tmy, &shader_matrix);
}

SkShader::TileMode ComputeTileMode(float left,
                                   float right,
                                   float min,
                                   float max) {
  DCHECK(left < right);
  return left >= min && right <= max ? SkShader::kClamp_TileMode
                                     : SkShader::kRepeat_TileMode;
}

}  // anonymous namespace

void Image::DrawPattern(GraphicsContext& context,
                        const FloatRect& float_src_rect,
                        const FloatSize& scale,
                        const FloatPoint& phase,
                        SkBlendMode composite_op,
                        const FloatRect& dest_rect,
                        const FloatSize& repeat_spacing) {
  TRACE_EVENT0("skia", "Image::drawPattern");

  PaintImage image = PaintImageForCurrentFrame();
  if (!image)
    return;

  FloatRect norm_src_rect = float_src_rect;

  norm_src_rect.Intersect(FloatRect(0, 0, image.width(), image.height()));
  if (dest_rect.IsEmpty() || norm_src_rect.IsEmpty())
    return;  // nothing to draw

  SkMatrix local_matrix;
  // We also need to translate it such that the origin of the pattern is the
  // origin of the destination rect, which is what WebKit expects. Skia uses
  // the coordinate system origin as the base for the pattern. If WebKit wants
  // a shifted image, it will shift it from there using the localMatrix.
  const float adjusted_x = phase.X() + norm_src_rect.X() * scale.Width();
  const float adjusted_y = phase.Y() + norm_src_rect.Y() * scale.Height();
  local_matrix.setTranslate(SkFloatToScalar(adjusted_x),
                            SkFloatToScalar(adjusted_y));

  // Because no resizing occurred, the shader transform should be
  // set to the pattern's transform, which just includes scale.
  local_matrix.preScale(scale.Width(), scale.Height());

  // Fetch this now as subsetting may swap the image.
  auto image_id = image.GetSkImage()->uniqueID();

  image = PaintImageBuilder::WithCopy(std::move(image))
              .make_subset(EnclosingIntRect(norm_src_rect))
              .TakePaintImage();
  if (!image)
    return;

  const FloatSize tile_size(
      image.width() * scale.Width() + repeat_spacing.Width(),
      image.height() * scale.Height() + repeat_spacing.Height());
  const auto tmx = ComputeTileMode(dest_rect.X(), dest_rect.MaxX(), adjusted_x,
                                   adjusted_x + tile_size.Width());
  const auto tmy = ComputeTileMode(dest_rect.Y(), dest_rect.MaxY(), adjusted_y,
                                   adjusted_y + tile_size.Height());

  PaintFlags flags = context.FillFlags();
  flags.setColor(SK_ColorBLACK);
  flags.setBlendMode(composite_op);
  flags.setFilterQuality(
      context.ComputeFilterQuality(this, dest_rect, norm_src_rect));
  flags.setAntiAlias(context.ShouldAntialias());
  flags.setShader(
      CreatePatternShader(image, local_matrix, flags,
                          FloatSize(repeat_spacing.Width() / scale.Width(),
                                    repeat_spacing.Height() / scale.Height()),
                          tmx, tmy));
  // If the shader could not be instantiated (e.g. non-invertible matrix),
  // draw transparent.
  // Note: we can't simply bail, because of arbitrary blend mode.
  if (!flags.HasShader())
    flags.setColor(SK_ColorTRANSPARENT);

  context.DrawRect(dest_rect, flags);

  if (CurrentFrameIsLazyDecoded())
    PlatformInstrumentation::DidDrawLazyPixelRef(image_id);
}

scoped_refptr<Image> Image::ImageForDefaultFrame() {
  scoped_refptr<Image> image(this);

  return image;
}

PaintImageBuilder Image::CreatePaintImageBuilder() {
  auto animation_type = MaybeAnimated() ? PaintImage::AnimationType::ANIMATED
                                        : PaintImage::AnimationType::STATIC;
  return PaintImageBuilder::WithDefault()
      .set_id(stable_image_id_)
      .set_animation_type(animation_type)
      .set_is_multipart(is_multipart_);
}

bool Image::ApplyShader(PaintFlags& flags, const SkMatrix& local_matrix) {
  // Default shader impl: attempt to build a shader based on the current frame
  // SkImage.
  PaintImage image = PaintImageForCurrentFrame();
  if (!image)
    return false;

  flags.setShader(PaintShader::MakeImage(image, SkShader::kRepeat_TileMode,
                                         SkShader::kRepeat_TileMode,
                                         &local_matrix));
  if (!flags.HasShader())
    return false;

  // Animation is normally refreshed in draw() impls, which we don't call when
  // painting via shaders.
  StartAnimation();

  return true;
}

FloatRect Image::ComputeTileContaining(const FloatPoint& point,
                                       const FloatSize& tile_size,
                                       const FloatPoint& tile_phase,
                                       const FloatSize& tile_spacing) {
  const FloatSize actual_tile_size(tile_size + tile_spacing);
  return FloatRect(
      FloatPoint(point.X() + fmodf(-tile_phase.X(), actual_tile_size.Width()),
                 point.Y() + fmodf(-tile_phase.Y(), actual_tile_size.Height())),
      tile_size);
}

FloatRect Image::ComputeSubsetForTile(const FloatRect& tile,
                                      const FloatRect& dest,
                                      const FloatSize& image_size) {
  DCHECK(tile.Contains(dest));

  const FloatSize scale(tile.Width() / image_size.Width(),
                        tile.Height() / image_size.Height());

  FloatRect subset = dest;
  subset.SetX((dest.X() - tile.X()) / scale.Width());
  subset.SetY((dest.Y() - tile.Y()) / scale.Height());
  subset.SetWidth(dest.Width() / scale.Width());
  subset.SetHeight(dest.Height() / scale.Height());

  return subset;
}

}  // namespace blink
