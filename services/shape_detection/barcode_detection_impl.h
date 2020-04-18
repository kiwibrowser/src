// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_BARCODE_DETECTION_IMPL_H_
#define SERVICES_SHAPE_DETECTION_BARCODE_DETECTION_IMPL_H_

#include "services/shape_detection/public/mojom/barcodedetection.mojom.h"

namespace shape_detection {

class BarcodeDetectionImpl {
 public:
  static void Create(shape_detection::mojom::BarcodeDetectionRequest request);

 private:
  ~BarcodeDetectionImpl() = default;

  DISALLOW_COPY_AND_ASSIGN(BarcodeDetectionImpl);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_BARCODE_DETECTION_IMPL_H_
