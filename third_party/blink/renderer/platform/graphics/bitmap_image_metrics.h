// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_BITMAP_IMAGE_METRICS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_BITMAP_IMAGE_METRICS_H_

#include "third_party/blink/renderer/platform/graphics/image_orientation.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

struct skcms_ICCProfile;

namespace blink {

class PLATFORM_EXPORT BitmapImageMetrics {
  STATIC_ONLY(BitmapImageMetrics);

 public:
  // Values synced with 'DecodedImageType' in
  // src/tools/metrics/histograms/histograms.xml
  enum DecodedImageType {
    kImageUnknown = 0,
    kImageJPEG = 1,
    kImagePNG = 2,
    kImageGIF = 3,
    kImageWebP = 4,
    kImageICO = 5,
    kImageBMP = 6,
    kDecodedImageTypeEnumEnd = kImageBMP + 1
  };

  enum Gamma {
    // Values synced with 'Gamma' in src/tools/metrics/histograms/histograms.xml
    kGammaLinear = 0,
    kGammaSRGB = 1,
    kGamma2Dot2 = 2,
    kGammaNonStandard = 3,
    kGammaNull = 4,
    kGammaFail = 5,
    kGammaInvalid = 6,
    kGammaExponent = 7,
    kGammaTable = 8,
    kGammaParametric = 9,
    kGammaNamed = 10,
    kGammaEnd = kGammaNamed + 1,
  };

  static void CountDecodedImageType(const String& type);
  static void CountImageOrientation(const ImageOrientationEnum);
  static void CountImageGammaAndGamut(const skcms_ICCProfile*);

 private:
  static Gamma GetColorSpaceGamma(const skcms_ICCProfile*);
};

}  // namespace blink

#endif
