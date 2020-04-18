// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/bitmap_image_metrics.h"

#include "third_party/blink/renderer/platform/graphics/color_space_gamut.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"
#include "third_party/skia/third_party/skcms/skcms.h"

namespace blink {

void BitmapImageMetrics::CountDecodedImageType(const String& type) {
  DecodedImageType decoded_image_type =
      type == "jpg"
          ? kImageJPEG
          : type == "png"
                ? kImagePNG
                : type == "gif"
                      ? kImageGIF
                      : type == "webp"
                            ? kImageWebP
                            : type == "ico"
                                  ? kImageICO
                                  : type == "bmp"
                                        ? kImageBMP
                                        : DecodedImageType::kImageUnknown;

  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      EnumerationHistogram, decoded_image_type_histogram,
      ("Blink.DecodedImageType", kDecodedImageTypeEnumEnd));
  decoded_image_type_histogram.Count(decoded_image_type);
}

void BitmapImageMetrics::CountImageOrientation(
    const ImageOrientationEnum orientation) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      EnumerationHistogram, orientation_histogram,
      ("Blink.DecodedImage.Orientation", kImageOrientationEnumEnd));
  orientation_histogram.Count(orientation);
}

void BitmapImageMetrics::CountImageGammaAndGamut(
    const skcms_ICCProfile* color_profile) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(EnumerationHistogram, gamma_named_histogram,
                                  ("Blink.ColorSpace.Source", kGammaEnd));
  gamma_named_histogram.Count(GetColorSpaceGamma(color_profile));

  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      EnumerationHistogram, gamut_named_histogram,
      ("Blink.ColorGamut.Source", static_cast<int>(ColorSpaceGamut::kEnd)));
  gamut_named_histogram.Count(
      static_cast<int>(ColorSpaceUtilities::GetColorSpaceGamut(color_profile)));
}

BitmapImageMetrics::Gamma BitmapImageMetrics::GetColorSpaceGamma(
    const skcms_ICCProfile* color_profile) {
  Gamma gamma = kGammaNull;
  if (color_profile) {
    if (skcms_TRCs_AreApproximateInverse(
            color_profile, skcms_sRGB_Inverse_TransferFunction())) {
      gamma = kGammaSRGB;
    } else if (skcms_TRCs_AreApproximateInverse(
                   color_profile, skcms_Identity_TransferFunction())) {
      gamma = kGammaLinear;
    } else {
      gamma = kGammaNonStandard;
    }
  }
  return gamma;
}

}  // namespace blink
