// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SHAPEDETECTION_BARCODE_DETECTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SHAPEDETECTION_BARCODE_DETECTOR_H_

#include "services/shape_detection/public/mojom/barcodedetection.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/shapedetection/shape_detector.h"

namespace blink {

class ExecutionContext;

class MODULES_EXPORT BarcodeDetector final : public ShapeDetector {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static BarcodeDetector* Create(ExecutionContext*);

  void Trace(blink::Visitor*) override;

 private:
  explicit BarcodeDetector(ExecutionContext*);
  ~BarcodeDetector() override = default;

  ScriptPromise DoDetect(ScriptPromiseResolver*, SkBitmap) override;
  void OnDetectBarcodes(
      ScriptPromiseResolver*,
      Vector<shape_detection::mojom::blink::BarcodeDetectionResultPtr>);
  void OnBarcodeServiceConnectionError();

  shape_detection::mojom::blink::BarcodeDetectionPtr barcode_service_;

  HeapHashSet<Member<ScriptPromiseResolver>> barcode_service_requests_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SHAPEDETECTION_BARCODE_DETECTOR_H_
