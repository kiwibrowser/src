// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_TEXT_DETECTION_IMPL_WIN_H_
#define SERVICES_SHAPE_DETECTION_TEXT_DETECTION_IMPL_WIN_H_

#include <windows.graphics.imaging.h>
#include <windows.media.ocr.h>
#include <wrl/client.h>
#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/detection_utils_win.h"
#include "services/shape_detection/public/mojom/textdetection.mojom.h"

class SkBitmap;

namespace shape_detection {

using ABI::Windows::Graphics::Imaging::ISoftwareBitmapStatics;
using ABI::Windows::Media::Ocr::IOcrEngine;
using ABI::Windows::Media::Ocr::IOcrResult;
using ABI::Windows::Media::Ocr::OcrResult;

class TextDetectionImplWin : public mojom::TextDetection {
 public:
  TextDetectionImplWin(
      Microsoft::WRL::ComPtr<IOcrEngine> ocr_engine,
      Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory);
  ~TextDetectionImplWin() override;

  // mojom::TextDetection implementation.
  void Detect(const SkBitmap& bitmap,
              mojom::TextDetection::DetectCallback callback) override;

  void SetBinding(mojo::StrongBindingPtr<mojom::TextDetection> binding) {
    binding_ = std::move(binding);
  }

 private:
  Microsoft::WRL::ComPtr<IOcrEngine> ocr_engine_;
  Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory_;
  DetectCallback recognize_text_callback_;
  mojo::StrongBindingPtr<mojom::TextDetection> binding_;

  HRESULT BeginDetect(const SkBitmap& bitmap);
  std::vector<mojom::TextDetectionResultPtr> BuildTextDetectionResult(
      AsyncOperation<OcrResult>::IAsyncOperationPtr async_op);
  void OnTextDetected(Microsoft::WRL::ComPtr<ISoftwareBitmap> win_bitmap,
                      AsyncOperation<OcrResult>::IAsyncOperationPtr async_op);

  base::WeakPtrFactory<TextDetectionImplWin> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TextDetectionImplWin);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_TEXT_DETECTION_IMPL_WIN_H_
