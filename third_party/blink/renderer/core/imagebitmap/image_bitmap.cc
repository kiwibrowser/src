// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/imagebitmap/image_bitmap.h"

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/html/canvas/image_data.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/offscreencanvas/offscreen_canvas.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/graphics/canvas_color_params.h"
#include "third_party/blink/renderer/platform/graphics/canvas_resource_provider.h"
#include "third_party/blink/renderer/platform/graphics/skia/skia_utils.h"
#include "third_party/blink/renderer/platform/image-decoders/image_decoder.h"
#include "third_party/blink/renderer/platform/threading/background_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/checked_numeric.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpaceXformCanvas.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSwizzle.h"
#include "third_party/skia/include/effects/SkColorFilterImageFilter.h"

namespace blink {

constexpr const char* kImageOrientationFlipY = "flipY";
constexpr const char* kImageBitmapOptionNone = "none";
constexpr const char* kImageBitmapOptionDefault = "default";
constexpr const char* kImageBitmapOptionPremultiply = "premultiply";
constexpr const char* kImageBitmapOptionResizeQualityHigh = "high";
constexpr const char* kImageBitmapOptionResizeQualityMedium = "medium";
constexpr const char* kImageBitmapOptionResizeQualityPixelated = "pixelated";
constexpr const char* kSRGBImageBitmapColorSpaceConversion = "srgb";
constexpr const char* kLinearRGBImageBitmapColorSpaceConversion = "linear-rgb";
constexpr const char* kP3ImageBitmapColorSpaceConversion = "p3";
constexpr const char* kRec2020ImageBitmapColorSpaceConversion = "rec2020";

namespace {

// The following two functions are helpers used in cropImage
static inline IntRect NormalizeRect(const IntRect& rect) {
  return IntRect(std::min(rect.X(), rect.MaxX()),
                 std::min(rect.Y(), rect.MaxY()),
                 std::max(rect.Width(), -rect.Width()),
                 std::max(rect.Height(), -rect.Height()));
}

ImageBitmap::ParsedOptions ParseOptions(const ImageBitmapOptions& options,
                                        base::Optional<IntRect> crop_rect,
                                        IntSize source_size) {
  ImageBitmap::ParsedOptions parsed_options;
  if (options.imageOrientation() == kImageOrientationFlipY) {
    parsed_options.flip_y = true;
  } else {
    parsed_options.flip_y = false;
    DCHECK(options.imageOrientation() == kImageBitmapOptionNone);
  }
  if (options.premultiplyAlpha() == kImageBitmapOptionNone) {
    parsed_options.premultiply_alpha = false;
  } else {
    parsed_options.premultiply_alpha = true;
    DCHECK(options.premultiplyAlpha() == kImageBitmapOptionDefault ||
           options.premultiplyAlpha() == kImageBitmapOptionPremultiply);
  }

  parsed_options.has_color_space_conversion =
      (options.colorSpaceConversion() != kImageBitmapOptionNone);
  parsed_options.color_params.SetCanvasColorSpace(kSRGBCanvasColorSpace);
  if (options.colorSpaceConversion() != kSRGBImageBitmapColorSpaceConversion &&
      options.colorSpaceConversion() != kImageBitmapOptionNone &&
      options.colorSpaceConversion() != kImageBitmapOptionDefault) {
    parsed_options.color_params.SetCanvasPixelFormat(kF16CanvasPixelFormat);
    if (options.colorSpaceConversion() ==
        kLinearRGBImageBitmapColorSpaceConversion) {
      parsed_options.color_params.SetCanvasColorSpace(kSRGBCanvasColorSpace);
    } else if (options.colorSpaceConversion() ==
               kP3ImageBitmapColorSpaceConversion) {
      parsed_options.color_params.SetCanvasColorSpace(kP3CanvasColorSpace);
    } else if (options.colorSpaceConversion() ==
               kRec2020ImageBitmapColorSpaceConversion) {
      parsed_options.color_params.SetCanvasColorSpace(kRec2020CanvasColorSpace);
    } else {
      NOTREACHED()
          << "Invalid ImageBitmap creation attribute colorSpaceConversion: "
          << options.colorSpaceConversion();
    }
  }

  int source_width = source_size.Width();
  int source_height = source_size.Height();
  if (!crop_rect) {
    parsed_options.crop_rect = IntRect(0, 0, source_width, source_height);
  } else {
    parsed_options.crop_rect = NormalizeRect(*crop_rect);
  }
  if (!options.hasResizeWidth() && !options.hasResizeHeight()) {
    parsed_options.resize_width = parsed_options.crop_rect.Width();
    parsed_options.resize_height = parsed_options.crop_rect.Height();
  } else if (options.hasResizeWidth() && options.hasResizeHeight()) {
    parsed_options.resize_width = options.resizeWidth();
    parsed_options.resize_height = options.resizeHeight();
  } else if (options.hasResizeWidth() && !options.hasResizeHeight()) {
    parsed_options.resize_width = options.resizeWidth();
    parsed_options.resize_height = ceil(
        static_cast<float>(options.resizeWidth()) /
        parsed_options.crop_rect.Width() * parsed_options.crop_rect.Height());
  } else {
    parsed_options.resize_height = options.resizeHeight();
    parsed_options.resize_width = ceil(
        static_cast<float>(options.resizeHeight()) /
        parsed_options.crop_rect.Height() * parsed_options.crop_rect.Width());
  }
  if (static_cast<int>(parsed_options.resize_width) ==
          parsed_options.crop_rect.Width() &&
      static_cast<int>(parsed_options.resize_height) ==
          parsed_options.crop_rect.Height()) {
    parsed_options.should_scale_input = false;
    return parsed_options;
  }
  parsed_options.should_scale_input = true;

  if (options.resizeQuality() == kImageBitmapOptionResizeQualityHigh)
    parsed_options.resize_quality = kHigh_SkFilterQuality;
  else if (options.resizeQuality() == kImageBitmapOptionResizeQualityMedium)
    parsed_options.resize_quality = kMedium_SkFilterQuality;
  else if (options.resizeQuality() == kImageBitmapOptionResizeQualityPixelated)
    parsed_options.resize_quality = kNone_SkFilterQuality;
  else
    parsed_options.resize_quality = kLow_SkFilterQuality;
  return parsed_options;
}

// The function dstBufferSizeHasOverflow() is being called at the beginning of
// each ImageBitmap() constructor, which makes sure that doing
// width * height * bytesPerPixel will never overflow unsigned.
bool DstBufferSizeHasOverflow(const ImageBitmap::ParsedOptions& options) {
  CheckedNumeric<unsigned> total_bytes = options.crop_rect.Width();
  total_bytes *= options.crop_rect.Height();
  total_bytes *=
      SkColorTypeBytesPerPixel(options.color_params.GetSkColorType());
  if (!total_bytes.IsValid())
    return true;

  if (!options.should_scale_input)
    return false;
  total_bytes = options.resize_width;
  total_bytes *= options.resize_height;
  total_bytes *=
      SkColorTypeBytesPerPixel(options.color_params.GetSkColorType());
  if (!total_bytes.IsValid())
    return true;

  return false;
}

SkImageInfo GetSkImageInfo(sk_sp<SkImage> skia_image) {
  SkColorType color_type = kN32_SkColorType;
  if (skia_image->colorType() == kRGBA_F16_SkColorType ||
      (skia_image->colorSpace() && skia_image->colorSpace()->gammaIsLinear())) {
    color_type = kRGBA_F16_SkColorType;
  }
  return SkImageInfo::Make(skia_image->width(), skia_image->height(),
                           color_type, skia_image->alphaType(),
                           skia_image->refColorSpace());
}

SkImageInfo GetSkImageInfo(const scoped_refptr<StaticBitmapImage>& image) {
  return GetSkImageInfo(image->PaintImageForCurrentFrame().GetSkImage());
}

// This function results in a readback due to using SkImage::readPixels().
// Returns transparent black pixels if the input SkImageInfo.bounds() does
// not intersect with the input image boundaries.
scoped_refptr<Uint8Array> CopyImageData(
    const scoped_refptr<StaticBitmapImage>& input,
    const SkImageInfo& info,
    const unsigned x = 0,
    const unsigned y = 0) {
  if (info.isEmpty())
    return nullptr;
  sk_sp<SkImage> sk_image = input->PaintImageForCurrentFrame().GetSkImage();
  if (sk_image->bounds().isEmpty())
    return nullptr;
  scoped_refptr<ArrayBuffer> dst_buffer =
      ArrayBuffer::CreateOrNull(info.computeMinByteSize(), 1);
  if (!dst_buffer)
    return nullptr;
  unsigned byte_length = dst_buffer->ByteLength();
  scoped_refptr<Uint8Array> dst_pixels =
      Uint8Array::Create(std::move(dst_buffer), 0, byte_length);
  if (!dst_pixels)
    return nullptr;
  bool read_pixels_successful =
      sk_image->readPixels(info, dst_pixels->Data(), info.minRowBytes(), x, y);
  DCHECK(read_pixels_successful);
  if (!read_pixels_successful)
    return nullptr;
  return dst_pixels;
}

scoped_refptr<Uint8Array> CopyImageData(
    const scoped_refptr<StaticBitmapImage>& input) {
  SkImageInfo info = GetSkImageInfo(input);
  return CopyImageData(std::move(input), info);
}

static inline bool ShouldAvoidPremul(
    const ImageBitmap::ParsedOptions& options) {
  return options.source_is_unpremul && !options.premultiply_alpha;
}

scoped_refptr<StaticBitmapImage> FlipImageVertically(
    scoped_refptr<StaticBitmapImage> input,
    const ImageBitmap::ParsedOptions& parsed_options) {
  sk_sp<SkImage> image = input->PaintImageForCurrentFrame().GetSkImage();

  if (ShouldAvoidPremul(parsed_options)) {
    // Unpremul code path results in a GPU readback if |input| is texture
    // backed since CopyImageData() uses  SkImage::readPixels() to extract the
    // pixels from SkImage.
    scoped_refptr<Uint8Array> image_pixels = CopyImageData(input);
    if (!image_pixels)
      return nullptr;
    SkImageInfo info = GetSkImageInfo(input);
    unsigned image_row_bytes = info.width() * info.bytesPerPixel();
    for (int i = 0; i < info.height() / 2; i++) {
      unsigned top_first_element = i * image_row_bytes;
      unsigned top_last_element = (i + 1) * image_row_bytes;
      unsigned bottom_first_element = (info.height() - 1 - i) * image_row_bytes;
      std::swap_ranges(image_pixels->Data() + top_first_element,
                       image_pixels->Data() + top_last_element,
                       image_pixels->Data() + bottom_first_element);
    }
    return StaticBitmapImage::Create(std::move(image_pixels), info);
  }

  // Since we are allowed to premul the input image if needed, we can use Skia
  // to flip the image by drawing it on a surface. If the image is premul, we
  // can use both accelerated and software surfaces. If the image is unpremul,
  // we have to use software surfaces.
  sk_sp<SkSurface> surface = nullptr;
  if (image->isTextureBacked() && image->alphaType() == kPremul_SkAlphaType) {
    GrContext* context =
        input->ContextProviderWrapper()->ContextProvider()->GetGrContext();
    if (context) {
      surface = SkSurface::MakeRenderTarget(context, SkBudgeted::kNo,
                                            GetSkImageInfo(input));
    }
  }
  if (!surface)
    surface = SkSurface::MakeRaster(GetSkImageInfo(input));
  if (!surface)
    return nullptr;
  SkCanvas* canvas = surface->getCanvas();
  canvas->scale(1, -1);
  canvas->translate(0, -input->height());
  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kSrc);
  canvas->drawImage(image.get(), 0, 0, &paint);
  return StaticBitmapImage::Create(surface->makeImageSnapshot(),
                                   input->ContextProviderWrapper());
}

scoped_refptr<StaticBitmapImage> GetImageWithAlphaDisposition(
    scoped_refptr<StaticBitmapImage>&& image,
    AlphaDisposition alpha_disposition) {
  DCHECK(alpha_disposition != kDontChangeAlpha);
  if (alpha_disposition == kDontChangeAlpha)
    return std::move(image);
  SkAlphaType alpha_type = (alpha_disposition == kPremultiplyAlpha)
                               ? kPremul_SkAlphaType
                               : kUnpremul_SkAlphaType;
  sk_sp<SkImage> skia_image = image->PaintImageForCurrentFrame().GetSkImage();
  if (skia_image->alphaType() == alpha_type)
    return std::move(image);

  // Premul/unpremul are performed in gamma-corrected space, using arithmetic
  // that assumes linear space. It is an incorrect implementation that has
  // become the de facto standard on the web. Therefore, the legacy behavior
  // must be maintained to ensure backward compatibility. Historically, passing
  // nullptr as the color space to SkImage::readPixels() enforces alpha
  // disposition to be done in non-linear space, using arithmetic that assumes
  // otherwise, but we want to avoid passing nullptr color space
  // (crbug.com/811318). Therefore, to premul, we draw on a surface or use
  // SkColorSpaceXform, and to unpremul, we read back the pixels and unpremul
  // manually. Unpremul results in a GPU readback if |image| is texture backed,
  // which cannot be avoided for now (crbug.com/740197).

  SkImageInfo info = GetSkImageInfo(image.get()).makeAlphaType(alpha_type);

  // Manual alpha disposition is needed if the image has a non-linear gamma
  // color space but still uses 8888 pixel storage format. In this case, we
  // ignore the gamma transfer and premul/unpremul the pixels in linear space.
  bool manual_alpha_disposition_needed =
      skia_image->colorSpace() &&
      skia_image->colorType() != kRGBA_F16_SkColorType &&
      !skia_image->colorSpace()->gammaIsLinear();

  if (alpha_type == kUnpremul_SkAlphaType) {
    scoped_refptr<Uint8Array> dst_pixels = nullptr;
    if (manual_alpha_disposition_needed) {
      dst_pixels = CopyImageData(image);
      if (!dst_pixels)
        return nullptr;
      int alpha = 0;
      for (unsigned i = 0; i < image->Size().Area(); i++) {
        alpha = dst_pixels->Data()[i * 4 + 3];
        dst_pixels->Data()[i * 4] =
            std::round(dst_pixels->Data()[i * 4] * 255.0 / alpha);
        dst_pixels->Data()[i * 4 + 1] =
            std::round(dst_pixels->Data()[i * 4 + 1] * 255.0 / alpha);
        dst_pixels->Data()[i * 4 + 2] =
            std::round(dst_pixels->Data()[i * 4 + 2] * 255.0 / alpha);
      }
    } else {
      dst_pixels = CopyImageData(image, info);
      if (!dst_pixels)
        return nullptr;
    }
    return StaticBitmapImage::Create(std::move(dst_pixels), info);
  }

  // We prefer to draw on a canvas to premul, since it allows us to avoid GPU
  // readback if |image| is texture backed. However, since there is no API
  // for tagging a texture backed SkImage with a color space without touching
  // the pixels, whe still need to fall back to manual premul in linear space
  // when necessary, which may result in a GPU read back.
  if (manual_alpha_disposition_needed) {
    scoped_refptr<Uint8Array> dst_pixels = CopyImageData(image);
    if (!dst_pixels)
      return nullptr;
    sk_sp<SkColorSpace> color_space = SkColorSpace::MakeSRGBLinear();
    SkColorSpaceXform::ColorFormat color_format =
        SkColorSpaceXform::kRGBA_8888_ColorFormat;
    if (info.colorType() == kRGBA_F16_SkColorType)
      color_format = SkColorSpaceXform::kRGBA_F16_ColorFormat;
    SkColorSpaceXform::Apply(
        color_space.get(), color_format, (void*)(dst_pixels->Data()),
        color_space.get(), color_format, (void*)(dst_pixels->Data()),
        image->Size().Area(), SkColorSpaceXform::kPremul_AlphaOp);
    return StaticBitmapImage::Create(std::move(dst_pixels), info);
  }

  sk_sp<SkSurface> surface = nullptr;
  if (image->IsTextureBacked()) {
    GrContext* context =
        image->ContextProviderWrapper()->ContextProvider()->GetGrContext();
    if (context)
      surface = SkSurface::MakeRenderTarget(context, SkBudgeted::kNo, info);
  }
  if (!surface)
    surface = SkSurface::MakeRaster(info);
  if (!surface)
    return nullptr;
  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kSrc);
  surface->getCanvas()->drawImage(skia_image.get(), 0, 0, &paint);
  return StaticBitmapImage::Create(surface->makeImageSnapshot(),
                                   image->ContextProviderWrapper());
}

void freePixels(const void*, void* pixels) {
  static_cast<Uint8Array*>(pixels)->Release();
}

scoped_refptr<StaticBitmapImage> ScaleImage(
    scoped_refptr<StaticBitmapImage>&& image,
    const ImageBitmap::ParsedOptions& parsed_options) {
  auto sk_image = image->PaintImageForCurrentFrame().GetSkImage();
  auto image_info = GetSkImageInfo(image).makeWH(parsed_options.resize_width,
                                                 parsed_options.resize_height);
  sk_sp<SkSurface> surface = nullptr;
  sk_sp<SkImage> resized_sk_image = nullptr;

  // Try to avoid GPU read back by drawing accelerated premul image on an
  // accelerated surface.
  if (!ShouldAvoidPremul(parsed_options) && image->IsTextureBacked() &&
      sk_image->alphaType() == kPremul_SkAlphaType) {
    GrContext* context =
        image->ContextProviderWrapper()->ContextProvider()->GetGrContext();
    if (context) {
      surface =
          SkSurface::MakeRenderTarget(context, SkBudgeted::kNo, image_info);
    }
    if (surface) {
      SkPaint paint;
      paint.setFilterQuality(parsed_options.resize_quality);
      surface->getCanvas()->drawImageRect(
          sk_image.get(),
          SkRect::MakeWH(parsed_options.resize_width,
                         parsed_options.resize_height),
          &paint, SkCanvas::SrcRectConstraint::kStrict_SrcRectConstraint);
      resized_sk_image = surface->makeImageSnapshot();
    }
  }

  if (!surface) {
    // Avoid sRGB transfer function by setting the color space to nullptr.
    if (image_info.colorSpace()->isSRGB())
      image_info = image_info.makeColorSpace(nullptr);
    scoped_refptr<ArrayBuffer> resized_buffer =
        ArrayBuffer::CreateOrNull(image_info.computeMinByteSize(), 1);
    if (!resized_buffer)
      return nullptr;
    scoped_refptr<Uint8Array> resized_pixels = Uint8Array::Create(
        std::move(resized_buffer), 0, image_info.computeMinByteSize());
    if (!resized_pixels)
      return nullptr;
    SkPixmap resized_pixmap(image_info, resized_pixels->Data(),
                            image_info.minRowBytes());
    sk_image->scalePixels(resized_pixmap, parsed_options.resize_quality);
    // Tag the resized Pixmap with the correct color space.
    resized_pixmap.setColorSpace(GetSkImageInfo(image).refColorSpace());

    Uint8Array* pixels = resized_pixels.get();
    if (pixels) {
      pixels->AddRef();
      resized_pixels = nullptr;
    }
    resized_sk_image =
        SkImage::MakeFromRaster(resized_pixmap, freePixels, pixels);
  }

  if (!resized_sk_image)
    return nullptr;
  return StaticBitmapImage::Create(resized_sk_image,
                                   image->ContextProviderWrapper());
}

scoped_refptr<StaticBitmapImage> ApplyColorSpaceConversion(
    scoped_refptr<StaticBitmapImage>&& image,
    ImageBitmap::ParsedOptions& options) {
  SkTransferFunctionBehavior transfer_function_behavior =
      SkTransferFunctionBehavior::kRespect;
  // We normally expect to respect transfer function. However, in two scenarios
  // we have to ignore the transfer function. First, when the source image is
  // unpremul. Second, when the source image is drawn using a
  // SkColorSpaceXformCanvas.
  sk_sp<SkImage> skia_image = image->PaintImageForCurrentFrame().GetSkImage();
  if (!skia_image->colorSpace() ||
      skia_image->alphaType() == kUnpremul_SkAlphaType)
    transfer_function_behavior = SkTransferFunctionBehavior::kIgnore;

  return image->ConvertToColorSpace(
      options.color_params.GetSkColorSpaceForSkSurfaces(),
      transfer_function_behavior);
}

scoped_refptr<StaticBitmapImage> MakeBlankImage(
    const ImageBitmap::ParsedOptions& parsed_options) {
  SkImageInfo info = SkImageInfo::Make(
      parsed_options.crop_rect.Width(), parsed_options.crop_rect.Height(),
      parsed_options.color_params.GetSkColorType(), kPremul_SkAlphaType,
      parsed_options.color_params.GetSkColorSpaceForSkSurfaces());
  if (parsed_options.should_scale_input) {
    info =
        info.makeWH(parsed_options.resize_width, parsed_options.resize_height);
  }
  sk_sp<SkSurface> surface = SkSurface::MakeRaster(info);
  if (!surface)
    return nullptr;
  return StaticBitmapImage::Create(surface->makeImageSnapshot());
}

}  // namespace

sk_sp<SkImage> ImageBitmap::GetSkImageFromDecoder(
    std::unique_ptr<ImageDecoder> decoder) {
  if (!decoder->FrameCount())
    return nullptr;
  ImageFrame* frame = decoder->DecodeFrameBufferAtIndex(0);
  if (!frame || frame->GetStatus() != ImageFrame::kFrameComplete)
    return nullptr;
  DCHECK(!frame->Bitmap().isNull() && !frame->Bitmap().empty());
  return frame->FinalizePixelsAndGetImage();
}

static scoped_refptr<StaticBitmapImage> CropImageAndApplyColorSpaceConversion(
    scoped_refptr<Image>&& image,
    ImageBitmap::ParsedOptions& parsed_options) {
  DCHECK(image);
  IntRect img_rect(IntPoint(), IntSize(image->width(), image->height()));
  const IntRect src_rect = Intersection(img_rect, parsed_options.crop_rect);

  // If cropRect doesn't intersect the source image, return a transparent black
  // image.
  if (src_rect.IsEmpty())
    return MakeBlankImage(parsed_options);

  sk_sp<SkImage> skia_image = image->PaintImageForCurrentFrame().GetSkImage();
  // Attempt to get raw unpremultiplied image data, executed only when
  // skia_image is premultiplied.
  if (!skia_image->isOpaque() && image->Data() &&
      skia_image->alphaType() == kPremul_SkAlphaType) {
    std::unique_ptr<ImageDecoder> decoder(ImageDecoder::Create(
        image->Data(), true,
        parsed_options.premultiply_alpha ? ImageDecoder::kAlphaPremultiplied
                                         : ImageDecoder::kAlphaNotPremultiplied,
        parsed_options.has_color_space_conversion ? ColorBehavior::Tag()
                                                  : ColorBehavior::Ignore()));
    if (!decoder)
      return nullptr;
    skia_image = ImageBitmap::GetSkImageFromDecoder(std::move(decoder));
    if (!skia_image)
      return nullptr;

    // In the case where the source image is lazy-decoded, image_ may not be in
    // a decoded state, we trigger it here.
    SkPixmap pixmap;
    if (!skia_image->isTextureBacked() && !skia_image->peekPixels(&pixmap)) {
      sk_sp<SkSurface> surface =
          SkSurface::MakeRaster(GetSkImageInfo(StaticBitmapImage::Create(
              skia_image, image->ContextProviderWrapper())));
      SkPaint paint;
      paint.setBlendMode(SkBlendMode::kSrc);
      surface->getCanvas()->drawImage(skia_image.get(), 0, 0, &paint);
      skia_image = surface->makeImageSnapshot();
    }
  }

  if (src_rect != img_rect)
    skia_image = skia_image->makeSubset(src_rect);

  scoped_refptr<StaticBitmapImage> result =
      StaticBitmapImage::Create(skia_image, image->ContextProviderWrapper());

  // down-scaling has higher priority than other tasks, up-scaling has lower.
  bool down_scaling =
      parsed_options.should_scale_input &&
      (parsed_options.resize_width * parsed_options.resize_height <
       result->Size().Area());
  bool up_scaling = parsed_options.should_scale_input && !down_scaling;

  // resize if down-scaling
  if (down_scaling) {
    result = ScaleImage(std::move(result), parsed_options);
    if (!result)
      return nullptr;
  }

  // flip if needed
  if (parsed_options.flip_y) {
    result = FlipImageVertically(std::move(result), parsed_options);
    if (!result)
      return nullptr;
  }

  // color convert if needed
  if (parsed_options.has_color_space_conversion) {
    result = ApplyColorSpaceConversion(std::move(result), parsed_options);
    if (!result)
      return nullptr;
  }

  // premultiply / unpremultiply if needed
  result = GetImageWithAlphaDisposition(std::move(result),
                                        parsed_options.premultiply_alpha
                                            ? kPremultiplyAlpha
                                            : kUnpremultiplyAlpha);
  // resize if up-scaling
  if (up_scaling) {
    result = ScaleImage(std::move(result), parsed_options);
    if (!result)
      return nullptr;
  }

  return result;
}

ImageBitmap::ImageBitmap(ImageElementBase* image,
                         base::Optional<IntRect> crop_rect,
                         Document* document,
                         const ImageBitmapOptions& options) {
  scoped_refptr<Image> input = image->CachedImage()->GetImage();
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, image->BitmapSourceSize());
  parsed_options.source_is_unpremul =
      (input->PaintImageForCurrentFrame().GetSkImage()->alphaType() ==
       kUnpremul_SkAlphaType);
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ =
      CropImageAndApplyColorSpaceConversion(std::move(input), parsed_options);
  if (!image_)
    return;

  image_->SetOriginClean(
      !image->WouldTaintOrigin(document->GetSecurityOrigin()));
}

ImageBitmap::ImageBitmap(HTMLVideoElement* video,
                         base::Optional<IntRect> crop_rect,
                         Document* document,
                         const ImageBitmapOptions& options) {
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, video->BitmapSourceSize());
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  std::unique_ptr<CanvasResourceProvider> resource_provider =
      CanvasResourceProvider::Create(
          IntSize(video->videoWidth(), video->videoHeight()),
          CanvasResourceProvider::kSoftwareResourceUsage);
  if (!resource_provider)
    return;

  video->PaintCurrentFrame(
      resource_provider->Canvas(),
      IntRect(IntPoint(), IntSize(video->videoWidth(), video->videoHeight())),
      nullptr);
  scoped_refptr<StaticBitmapImage> input = resource_provider->Snapshot();
  image_ = CropImageAndApplyColorSpaceConversion(input, parsed_options);
  if (!image_)
    return;

  image_->SetOriginClean(
      !video->WouldTaintOrigin(document->GetSecurityOrigin()));
}

ImageBitmap::ImageBitmap(HTMLCanvasElement* canvas,
                         base::Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  SourceImageStatus status;
  scoped_refptr<Image> image_input = canvas->GetSourceImageForCanvas(
      &status, kPreferAcceleration, FloatSize());
  if (status != kNormalSourceImageStatus)
    return;
  DCHECK(image_input->IsStaticBitmapImage());
  scoped_refptr<StaticBitmapImage> input =
      static_cast<StaticBitmapImage*>(image_input.get());

  ParsedOptions parsed_options = ParseOptions(
      options, crop_rect, IntSize(input->width(), input->height()));
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ =
      CropImageAndApplyColorSpaceConversion(std::move(input), parsed_options);
  if (!image_)
    return;

  image_->SetOriginClean(canvas->OriginClean());
}

ImageBitmap::ImageBitmap(OffscreenCanvas* offscreen_canvas,
                         base::Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  SourceImageStatus status;
  scoped_refptr<Image> raw_input = offscreen_canvas->GetSourceImageForCanvas(
      &status, kPreferNoAcceleration, FloatSize(offscreen_canvas->Size()));
  DCHECK(raw_input->IsStaticBitmapImage());
  scoped_refptr<StaticBitmapImage> input =
      static_cast<StaticBitmapImage*>(raw_input.get());
  raw_input = nullptr;

  if (status != kNormalSourceImageStatus)
    return;

  ParsedOptions parsed_options = ParseOptions(
      options, crop_rect, IntSize(input->width(), input->height()));
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ =
      CropImageAndApplyColorSpaceConversion(std::move(input), parsed_options);
  if (!image_)
    return;
  image_->SetOriginClean(offscreen_canvas->OriginClean());
}

ImageBitmap::ImageBitmap(const void* pixel_data,
                         uint32_t width,
                         uint32_t height,
                         bool is_image_bitmap_premultiplied,
                         bool is_image_bitmap_origin_clean,
                         const CanvasColorParams& color_params) {
  SkImageInfo info =
      SkImageInfo::Make(width, height, color_params.GetSkColorType(),
                        is_image_bitmap_premultiplied ? kPremul_SkAlphaType
                                                      : kUnpremul_SkAlphaType,
                        color_params.GetSkColorSpaceForSkSurfaces());
  SkPixmap pixmap(info, pixel_data, info.bytesPerPixel() * width);
  sk_sp<SkImage> raster_copy = SkImage::MakeRasterCopy(pixmap);
  if (!raster_copy)
    return;
  image_ = StaticBitmapImage::Create(std::move(raster_copy));
  if (!image_)
    return;
  image_->SetOriginClean(is_image_bitmap_origin_clean);
}

ImageBitmap::ImageBitmap(ImageData* data,
                         base::Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, data->BitmapSourceSize());
  // ImageData is always unpremul.
  parsed_options.source_is_unpremul = true;
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  IntRect data_src_rect = IntRect(IntPoint(), data->Size());
  IntRect src_rect = crop_rect
                         ? Intersection(parsed_options.crop_rect, data_src_rect)
                         : data_src_rect;

  // If cropRect doesn't intersect the source image, return a transparent black
  // image.
  if (src_rect.IsEmpty()) {
    image_ = MakeBlankImage(parsed_options);
    return;
  }

  // Copy / color convert the pixels
  scoped_refptr<ArrayBuffer> pixels_buffer = ArrayBuffer::CreateOrNull(
      src_rect.Size().Area(), parsed_options.color_params.BytesPerPixel());
  if (!pixels_buffer)
    return;
  unsigned byte_length = pixels_buffer->ByteLength();
  scoped_refptr<Uint8Array> image_pixels =
      Uint8Array::Create(std::move(pixels_buffer), 0, byte_length);
  if (!image_pixels)
    return;
  if (!data->ImageDataInCanvasColorSettings(
          parsed_options.color_params.ColorSpace(),
          parsed_options.color_params.PixelFormat(), image_pixels->Data(),
          kN32ColorType, &src_rect,
          parsed_options.premultiply_alpha ? kPremultiplyAlpha
                                           : kUnpremultiplyAlpha))
    return;

  // Create Image object
  SkImageInfo info = SkImageInfo::Make(
      src_rect.Width(), src_rect.Height(),
      parsed_options.color_params.GetSkColorType(),
      parsed_options.premultiply_alpha ? kPremul_SkAlphaType
                                       : kUnpremul_SkAlphaType,
      parsed_options.color_params.GetSkColorSpaceForSkSurfaces());
  image_ = StaticBitmapImage::Create(std::move(image_pixels), info);
  if (!image_)
    return;

  // down-scaling has higher priority than other tasks, up-scaling has lower.
  bool down_scaling =
      parsed_options.should_scale_input &&
      (parsed_options.resize_width * parsed_options.resize_height <
       image_->Size().Area());
  bool up_scaling = parsed_options.should_scale_input && !down_scaling;

  // resize if down-scaling
  if (down_scaling)
    image_ = ScaleImage(std::move(image_), parsed_options);
  if (!image_)
    return;

  // flip if needed
  if (parsed_options.flip_y)
    image_ = FlipImageVertically(std::move(image_), parsed_options);
  if (!image_)
    return;

  // resize if up-scaling
  if (up_scaling)
    image_ = ScaleImage(std::move(image_), parsed_options);
}

ImageBitmap::ImageBitmap(ImageBitmap* bitmap,
                         base::Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  scoped_refptr<StaticBitmapImage> input = bitmap->BitmapImage();
  if (!input)
    return;
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, input->Size());
  parsed_options.source_is_unpremul =
      (input->PaintImageForCurrentFrame().GetSkImage()->alphaType() ==
       kUnpremul_SkAlphaType);
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ =
      CropImageAndApplyColorSpaceConversion(std::move(input), parsed_options);
  if (!image_)
    return;

  image_->SetOriginClean(bitmap->OriginClean());
}

ImageBitmap::ImageBitmap(scoped_refptr<StaticBitmapImage> image,
                         base::Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  bool origin_clean = image->OriginClean();
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, image->Size());
  parsed_options.source_is_unpremul =
      (image->PaintImageForCurrentFrame().GetSkImage()->alphaType() ==
       kUnpremul_SkAlphaType);
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ =
      CropImageAndApplyColorSpaceConversion(std::move(image), parsed_options);
  if (!image_)
    return;

  image_->SetOriginClean(origin_clean);
}

ImageBitmap::ImageBitmap(scoped_refptr<StaticBitmapImage> image) {
  image_ = std::move(image);
}

scoped_refptr<StaticBitmapImage> ImageBitmap::Transfer() {
  DCHECK(!IsNeutered());
  is_neutered_ = true;
  image_->Transfer();
  return std::move(image_);
}

ImageBitmap::~ImageBitmap() = default;

ImageBitmap* ImageBitmap::Create(ImageElementBase* image,
                                 base::Optional<IntRect> crop_rect,
                                 Document* document,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(image, crop_rect, document, options);
}

ImageBitmap* ImageBitmap::Create(HTMLVideoElement* video,
                                 base::Optional<IntRect> crop_rect,
                                 Document* document,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(video, crop_rect, document, options);
}

ImageBitmap* ImageBitmap::Create(HTMLCanvasElement* canvas,
                                 base::Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(canvas, crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(OffscreenCanvas* offscreen_canvas,
                                 base::Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(offscreen_canvas, crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(ImageData* data,
                                 base::Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(data, crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(ImageBitmap* bitmap,
                                 base::Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(bitmap, crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(scoped_refptr<StaticBitmapImage> image,
                                 base::Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(std::move(image), crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(scoped_refptr<StaticBitmapImage> image) {
  return new ImageBitmap(std::move(image));
}

ImageBitmap* ImageBitmap::Create(const void* pixel_data,
                                 uint32_t width,
                                 uint32_t height,
                                 bool is_image_bitmap_premultiplied,
                                 bool is_image_bitmap_origin_clean,
                                 const CanvasColorParams& color_params) {
  return new ImageBitmap(pixel_data, width, height,
                         is_image_bitmap_premultiplied,
                         is_image_bitmap_origin_clean, color_params);
}

void ImageBitmap::ResolvePromiseOnOriginalThread(
    ScriptPromiseResolver* resolver,
    sk_sp<SkImage> skia_image,
    bool origin_clean,
    std::unique_ptr<ParsedOptions> parsed_options) {
  if (!skia_image) {
    resolver->Reject(
        ScriptValue(resolver->GetScriptState(),
                    v8::Null(resolver->GetScriptState()->GetIsolate())));
    return;
  }
  scoped_refptr<StaticBitmapImage> image =
      StaticBitmapImage::Create(std::move(skia_image));
  DCHECK(IsMainThread());
  if (!parsed_options->premultiply_alpha) {
    image = GetImageWithAlphaDisposition(std::move(image), kUnpremultiplyAlpha);
  }
  if (!image) {
    resolver->Reject(
        ScriptValue(resolver->GetScriptState(),
                    v8::Null(resolver->GetScriptState()->GetIsolate())));
    return;
  }
  image = ApplyColorSpaceConversion(std::move(image), *(parsed_options.get()));
  if (!image) {
    resolver->Reject(
        ScriptValue(resolver->GetScriptState(),
                    v8::Null(resolver->GetScriptState()->GetIsolate())));
    return;
  }
  ImageBitmap* bitmap = new ImageBitmap(image);
  bitmap->BitmapImage()->SetOriginClean(origin_clean);
  resolver->Resolve(bitmap);
}

void ImageBitmap::RasterizeImageOnBackgroundThread(
    ScriptPromiseResolver* resolver,
    sk_sp<PaintRecord> paint_record,
    const IntRect& dst_rect,
    bool origin_clean,
    std::unique_ptr<ParsedOptions> parsed_options) {
  DCHECK(!IsMainThread());
  // TODO (zakerinasab): crbug.com/768844
  // For now only SVG is decoded async so it is fine to assume the color space
  // is SRGB. When other sources are decoded async (crbug.com/580202), make sure
  // that proper color space is used in SkImageInfo to avoid clipping the gamut
  // of the image bitmap source.
  SkImageInfo info = SkImageInfo::MakeS32(dst_rect.Width(), dst_rect.Height(),
                                          kPremul_SkAlphaType);
  sk_sp<SkSurface> surface = SkSurface::MakeRaster(info);
  sk_sp<SkImage> skia_image;
  if (surface) {
    paint_record->Playback(surface->getCanvas());
    skia_image = surface->makeImageSnapshot();
  }
  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      Platform::Current()->MainThread()->GetTaskRunner();
  PostCrossThreadTask(*task_runner, FROM_HERE,
                      CrossThreadBind(&ResolvePromiseOnOriginalThread,
                                      WrapCrossThreadPersistent(resolver),
                                      std::move(skia_image), origin_clean,
                                      WTF::Passed(std::move(parsed_options))));
}

ScriptPromise ImageBitmap::CreateAsync(ImageElementBase* image,
                                       base::Optional<IntRect> crop_rect,
                                       Document* document,
                                       ScriptState* script_state,
                                       const ImageBitmapOptions& options) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  scoped_refptr<Image> input = image->CachedImage()->GetImage();
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, image->BitmapSourceSize());
  if (DstBufferSizeHasOverflow(parsed_options)) {
    resolver->Reject(
        ScriptValue(resolver->GetScriptState(),
                    v8::Null(resolver->GetScriptState()->GetIsolate())));
    return promise;
  }

  IntRect input_rect(IntPoint(), input->Size());
  const IntRect src_rect = Intersection(input_rect, parsed_options.crop_rect);

  // In the case when |crop_rect| doesn't intersect the source image, we return
  // a transparent black image, respecting the color_params but ignoring
  // poremultiply_alpha.
  if (src_rect.IsEmpty()) {
    ImageBitmap* bitmap = new ImageBitmap(MakeBlankImage(parsed_options));
    if (bitmap->BitmapImage()) {
      bitmap->BitmapImage()->SetOriginClean(
          !image->WouldTaintOrigin(document->GetSecurityOrigin()));
      resolver->Resolve(bitmap);
    } else {
      resolver->Reject(
          ScriptValue(resolver->GetScriptState(),
                      v8::Null(resolver->GetScriptState()->GetIsolate())));
    }
    return promise;
  }

  IntRect draw_src_rect(parsed_options.crop_rect);
  IntRect draw_dst_rect(0, 0, parsed_options.resize_width,
                        parsed_options.resize_height);
  sk_sp<PaintRecord> paint_record =
      input->PaintRecordForContainer(NullURL(), input->Size(), draw_src_rect,
                                     draw_dst_rect, parsed_options.flip_y);
  std::unique_ptr<ParsedOptions> passed_parsed_options =
      std::make_unique<ParsedOptions>(parsed_options);
  BackgroundTaskRunner::PostOnBackgroundThread(
      FROM_HERE,
      CrossThreadBind(&RasterizeImageOnBackgroundThread,
                      WrapCrossThreadPersistent(resolver),
                      std::move(paint_record), draw_dst_rect,
                      !image->WouldTaintOrigin(document->GetSecurityOrigin()),
                      WTF::Passed(std::move(passed_parsed_options))));
  return promise;
}

void ImageBitmap::close() {
  if (!image_ || is_neutered_)
    return;
  image_ = nullptr;
  is_neutered_ = true;
}

// static
ImageBitmap* ImageBitmap::Take(ScriptPromiseResolver*, sk_sp<SkImage> image) {
  return ImageBitmap::Create(StaticBitmapImage::Create(std::move(image)));
}

CanvasColorParams ImageBitmap::GetCanvasColorParams() {
  return CanvasColorParams(GetSkImageInfo(image_));
}

scoped_refptr<Uint8Array> ImageBitmap::CopyBitmapData(
    AlphaDisposition alpha_op,
    DataU8ColorType u8_color_type) {
  DCHECK(alpha_op != kDontChangeAlpha);
  SkImageInfo info = GetSkImageInfo(image_);
  auto color_type = info.colorType();
  if (color_type == kN32_SkColorType && u8_color_type == kRGBAColorType)
    color_type = kRGBA_8888_SkColorType;
  info =
      SkImageInfo::Make(width(), height(), color_type,
                        (alpha_op == kPremultiplyAlpha) ? kPremul_SkAlphaType
                                                        : kUnpremul_SkAlphaType,
                        info.refColorSpace());
  return CopyImageData(image_, info);
}

scoped_refptr<Uint8Array> ImageBitmap::CopyBitmapData() {
  return CopyImageData(image_);
}

unsigned long ImageBitmap::width() const {
  if (!image_)
    return 0;
  DCHECK_GT(image_->width(), 0);
  return image_->width();
}

unsigned long ImageBitmap::height() const {
  if (!image_)
    return 0;
  DCHECK_GT(image_->height(), 0);
  return image_->height();
}

bool ImageBitmap::IsAccelerated() const {
  return image_ && (image_->IsTextureBacked() || image_->HasMailbox());
}

IntSize ImageBitmap::Size() const {
  if (!image_)
    return IntSize();
  DCHECK_GT(image_->width(), 0);
  DCHECK_GT(image_->height(), 0);
  return IntSize(image_->width(), image_->height());
}

ScriptPromise ImageBitmap::CreateImageBitmap(
    ScriptState* script_state,
    EventTarget& event_target,
    base::Optional<IntRect> crop_rect,
    const ImageBitmapOptions& options) {
  return ImageBitmapSource::FulfillImageBitmap(
      script_state, Create(this, crop_rect, options));
}

scoped_refptr<Image> ImageBitmap::GetSourceImageForCanvas(
    SourceImageStatus* status,
    AccelerationHint,
    const FloatSize&) {
  *status = kNormalSourceImageStatus;
  if (!image_)
    return nullptr;
  if (image_->IsPremultiplied())
    return image_;
  // Skia does not support drawing unpremul SkImage on SkCanvas.
  // Premultiply and return.
  return GetImageWithAlphaDisposition(std::move(image_), kPremultiplyAlpha);
}

void ImageBitmap::AdjustDrawRects(FloatRect* src_rect,
                                  FloatRect* dst_rect) const {}

FloatSize ImageBitmap::ElementSize(const FloatSize&) const {
  return FloatSize(width(), height());
}

}  // namespace blink
