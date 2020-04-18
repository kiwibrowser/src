// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/shapedetection/barcode_detector.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_image_source.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/modules/imagecapture/point_2d.h"
#include "third_party/blink/renderer/modules/shapedetection/detected_barcode.h"

namespace blink {

BarcodeDetector* BarcodeDetector::Create(ExecutionContext* context) {
  return new BarcodeDetector(context);
}

BarcodeDetector::BarcodeDetector(ExecutionContext* context) : ShapeDetector() {
  auto request = mojo::MakeRequest(&barcode_service_);
  if (auto* interface_provider = context->GetInterfaceProvider()) {
    interface_provider->GetInterface(std::move(request));
  }

  barcode_service_.set_connection_error_handler(
      WTF::Bind(&BarcodeDetector::OnBarcodeServiceConnectionError,
                WrapWeakPersistent(this)));
}

ScriptPromise BarcodeDetector::DoDetect(ScriptPromiseResolver* resolver,
                                        SkBitmap bitmap) {
  ScriptPromise promise = resolver->Promise();
  if (!barcode_service_) {
    resolver->Reject(DOMException::Create(
        kNotSupportedError, "Barcode detection service unavailable."));
    return promise;
  }
  barcode_service_requests_.insert(resolver);
  barcode_service_->Detect(
      std::move(bitmap),
      WTF::Bind(&BarcodeDetector::OnDetectBarcodes, WrapPersistent(this),
                WrapPersistent(resolver)));
  return promise;
}

void BarcodeDetector::OnDetectBarcodes(
    ScriptPromiseResolver* resolver,
    Vector<shape_detection::mojom::blink::BarcodeDetectionResultPtr>
        barcode_detection_results) {
  DCHECK(barcode_service_requests_.Contains(resolver));
  barcode_service_requests_.erase(resolver);

  HeapVector<Member<DetectedBarcode>> detected_barcodes;
  for (const auto& barcode : barcode_detection_results) {
    HeapVector<Point2D> corner_points;
    for (const auto& corner_point : barcode->corner_points) {
      Point2D point;
      point.setX(corner_point.x);
      point.setY(corner_point.y);
      corner_points.push_back(point);
    }
    detected_barcodes.push_back(DetectedBarcode::Create(
        barcode->raw_value,
        DOMRect::Create(barcode->bounding_box.x, barcode->bounding_box.y,
                        barcode->bounding_box.width,
                        barcode->bounding_box.height),
        corner_points));
  }

  resolver->Resolve(detected_barcodes);
}

void BarcodeDetector::OnBarcodeServiceConnectionError() {
  for (const auto& request : barcode_service_requests_) {
    request->Reject(DOMException::Create(kNotSupportedError,
                                         "Barcode Detection not implemented."));
  }
  barcode_service_requests_.clear();
  barcode_service_.reset();
}

void BarcodeDetector::Trace(blink::Visitor* visitor) {
  ShapeDetector::Trace(visitor);
  visitor->Trace(barcode_service_requests_);
}

}  // namespace blink
