// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_provider_mac.h"

#include <memory>
#include <utility>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/face_detection_impl_mac.h"
#include "services/shape_detection/face_detection_impl_mac_vision.h"

namespace shape_detection {

FaceDetectionProviderMac::FaceDetectionProviderMac() = default;

FaceDetectionProviderMac::~FaceDetectionProviderMac() = default;

// static
void FaceDetectionProviderMac::Create(
    mojom::FaceDetectionProviderRequest request) {
  mojo::MakeStrongBinding(std::make_unique<FaceDetectionProviderMac>(),
                          std::move(request));
}

void FaceDetectionProviderMac::CreateFaceDetection(
    mojom::FaceDetectionRequest request,
    mojom::FaceDetectorOptionsPtr options) {
  // Vision Framework needs at least MAC OS X 10.13.
  if (@available(macOS 10.13, *)) {
    // Vision is more accurate than Core Image Framework, but it also needs more
    // processing time.
    if (!options->fast_mode) {
      auto impl = std::make_unique<FaceDetectionImplMacVision>();
      auto* impl_ptr = impl.get();
      impl_ptr->SetBinding(
          mojo::MakeStrongBinding(std::move(impl), std::move(request)));
      return;
    }
  }

  mojo::MakeStrongBinding(
      std::make_unique<FaceDetectionImplMac>(std::move(options)),
      std::move(request));
}

}  // namespace shape_detection
