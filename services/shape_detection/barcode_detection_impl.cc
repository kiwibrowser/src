// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/barcode_detection_impl.h"

namespace shape_detection {

// static
void BarcodeDetectionImpl::Create(
    shape_detection::mojom::BarcodeDetectionRequest request) {
  DLOG(ERROR) << "Platform not supported for Barcode Detection Service.";
}

}  // namespace shape_detection
