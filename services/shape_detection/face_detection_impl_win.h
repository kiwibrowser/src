// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_
#define SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_

#include <windows.foundation.h>
#include <windows.graphics.imaging.h>
#include <windows.media.faceanalysis.h>
#include <wrl/client.h>
#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/detection_utils_win.h"
#include "services/shape_detection/public/mojom/facedetection.mojom.h"

class SkBitmap;

namespace shape_detection {

using ABI::Windows::Foundation::Collections::IVector;

class FaceDetectionImplWin : public mojom::FaceDetection {
 public:
  using FaceDetector = ABI::Windows::Media::FaceAnalysis::FaceDetector;
  using IFaceDetector = ABI::Windows::Media::FaceAnalysis::IFaceDetector;
  using DetectedFace = ABI::Windows::Media::FaceAnalysis::DetectedFace;
  using IDetectedFace = ABI::Windows::Media::FaceAnalysis::IDetectedFace;
  using BitmapPixelFormat = ABI::Windows::Graphics::Imaging::BitmapPixelFormat;
  using ISoftwareBitmapStatics =
      ABI::Windows::Graphics::Imaging::ISoftwareBitmapStatics;

  FaceDetectionImplWin(
      Microsoft::WRL::ComPtr<IFaceDetector> face_detector,
      Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory,
      BitmapPixelFormat pixel_format);
  ~FaceDetectionImplWin() override;

  void SetBinding(mojo::StrongBindingPtr<mojom::FaceDetection> binding) {
    binding_ = std::move(binding);
  }

  // mojom::FaceDetection implementation.
  void Detect(const SkBitmap& bitmap,
              mojom::FaceDetection::DetectCallback callback) override;

 private:
  HRESULT BeginDetect(const SkBitmap& bitmap);
  std::vector<mojom::FaceDetectionResultPtr> BuildFaceDetectionResult(
      AsyncOperation<IVector<DetectedFace*>>::IAsyncOperationPtr async_op);
  void OnFaceDetected(
      Microsoft::WRL::ComPtr<ISoftwareBitmap> win_bitmap,
      AsyncOperation<IVector<DetectedFace*>>::IAsyncOperationPtr async_op);

  Microsoft::WRL::ComPtr<IFaceDetector> face_detector_;

  Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory_;
  BitmapPixelFormat pixel_format_;

  DetectCallback detected_face_callback_;
  mojo::StrongBindingPtr<mojom::FaceDetection> binding_;

  base::WeakPtrFactory<FaceDetectionImplWin> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FaceDetectionImplWin);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_
