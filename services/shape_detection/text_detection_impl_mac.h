// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_TEXT_DETECTION_IMPL_MAC_H_
#define SERVICES_SHAPE_DETECTION_TEXT_DETECTION_IMPL_MAC_H_

#include "base/mac/availability.h"
#include "base/mac/scoped_nsobject.h"
#include "services/shape_detection/public/mojom/textdetection.mojom.h"

@class CIDetector;

namespace shape_detection {

class API_AVAILABLE(macosx(10.11)) TextDetectionImplMac
    : public mojom::TextDetection {
 public:
  TextDetectionImplMac();
  ~TextDetectionImplMac() override;

  void Detect(const SkBitmap& bitmap,
              mojom::TextDetection::DetectCallback callback) override;

 private:
  base::scoped_nsobject<CIDetector> detector_;

  DISALLOW_COPY_AND_ASSIGN(TextDetectionImplMac);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_TEXT_DETECTION_IMPL_MAC_H_
