// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_MAC_H_
#define SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_MAC_H_

#include "base/macros.h"
#include "services/shape_detection/public/mojom/facedetection_provider.mojom.h"

namespace shape_detection {

// The FaceDetectionProviderMac class is a provider that binds an implementation
// of mojom::FaceDetection with Core Image or Vision Framework.
class FaceDetectionProviderMac
    : public shape_detection::mojom::FaceDetectionProvider {
 public:
  FaceDetectionProviderMac();
  ~FaceDetectionProviderMac() override;

  // Binds FaceDetection provider request to the implementation of
  // mojom::FaceDetectionProvider.
  static void Create(mojom::FaceDetectionProviderRequest request);

  void CreateFaceDetection(mojom::FaceDetectionRequest request,
                           mojom::FaceDetectorOptionsPtr options) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FaceDetectionProviderMac);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_MAC_H_
