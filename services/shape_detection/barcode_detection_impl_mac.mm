// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/barcode_detection_impl_mac.h"

#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/barcode_detection_impl.h"
#include "services/shape_detection/detection_utils_mac.h"

namespace shape_detection {

// static
void BarcodeDetectionImpl::Create(
    shape_detection::mojom::BarcodeDetectionRequest request) {
  // Barcode detection needs at least MAC OS X 10.10.
  if (@available(macOS 10.10, *)) {
    mojo::MakeStrongBinding(std::make_unique<BarcodeDetectionImplMac>(),
                            std::move(request));
  }
}

BarcodeDetectionImplMac::BarcodeDetectionImplMac() {
  NSDictionary* const options = @{CIDetectorAccuracy : CIDetectorAccuracyHigh};
  detector_.reset([[CIDetector detectorOfType:CIDetectorTypeQRCode
                                      context:nil
                                      options:options] retain]);
}

BarcodeDetectionImplMac::~BarcodeDetectionImplMac() {}

void BarcodeDetectionImplMac::Detect(const SkBitmap& bitmap,
                                     DetectCallback callback) {
  base::scoped_nsobject<CIImage> ci_image = CreateCIImageFromSkBitmap(bitmap);
  if (!ci_image) {
    std::move(callback).Run({});
    return;
  }

  NSArray* const features = [detector_ featuresInImage:ci_image];

  std::vector<mojom::BarcodeDetectionResultPtr> results;
  const int height = bitmap.height();
  for (CIQRCodeFeature* const f in features) {
    shape_detection::mojom::BarcodeDetectionResultPtr result =
        shape_detection::mojom::BarcodeDetectionResult::New();
    result->bounding_box = ConvertCGToGfxCoordinates(f.bounds, height);

    // Enumerate corner points starting from top-left in clockwise fashion:
    // https://wicg.github.io/shape-detection-api/#dom-detectedbarcode-cornerpoints
    result->corner_points.emplace_back(f.topLeft.x, height - f.topLeft.y);
    result->corner_points.emplace_back(f.topRight.x, height - f.topRight.y);
    result->corner_points.emplace_back(f.bottomRight.x,
                                       height - f.bottomRight.y);
    result->corner_points.emplace_back(f.bottomLeft.x, height - f.bottomLeft.y);

    result->raw_value = base::SysNSStringToUTF8(f.messageString);
    results.push_back(std::move(result));
  }
  std::move(callback).Run(std::move(results));
}

}  // namespace shape_detection
