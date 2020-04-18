// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/shapedetection/face_detector.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/shape_detection/public/mojom/facedetection_provider.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_image_source.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/modules/imagecapture/point_2d.h"
#include "third_party/blink/renderer/modules/shapedetection/detected_face.h"
#include "third_party/blink/renderer/modules/shapedetection/face_detector_options.h"
#include "third_party/blink/renderer/modules/shapedetection/landmark.h"
#include "third_party/blink/renderer/modules/shapedetection/shape_detection_type_converter.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

FaceDetector* FaceDetector::Create(ExecutionContext* context,
                                   const FaceDetectorOptions& options) {
  return new FaceDetector(context, options);
}

FaceDetector::FaceDetector(ExecutionContext* context,
                           const FaceDetectorOptions& options)
    : ShapeDetector() {
  auto face_detector_options =
      shape_detection::mojom::blink::FaceDetectorOptions::New();
  face_detector_options->max_detected_faces = options.maxDetectedFaces();
  face_detector_options->fast_mode = options.fastMode();

  shape_detection::mojom::blink::FaceDetectionProviderPtr provider;
  auto request = mojo::MakeRequest(&provider);
  if (auto* interface_provider = context->GetInterfaceProvider()) {
    interface_provider->GetInterface(std::move(request));
  }
  provider->CreateFaceDetection(mojo::MakeRequest(&face_service_),
                                std::move(face_detector_options));

  face_service_.set_connection_error_handler(WTF::Bind(
      &FaceDetector::OnFaceServiceConnectionError, WrapWeakPersistent(this)));
}

ScriptPromise FaceDetector::DoDetect(ScriptPromiseResolver* resolver,
                                     SkBitmap bitmap) {
  ScriptPromise promise = resolver->Promise();
  if (!face_service_) {
    resolver->Reject(DOMException::Create(
        kNotSupportedError, "Face detection service unavailable."));
    return promise;
  }
  face_service_requests_.insert(resolver);
  face_service_->Detect(
      std::move(bitmap),
      WTF::Bind(&FaceDetector::OnDetectFaces, WrapPersistent(this),
                WrapPersistent(resolver)));
  return promise;
}

void FaceDetector::OnDetectFaces(
    ScriptPromiseResolver* resolver,
    Vector<shape_detection::mojom::blink::FaceDetectionResultPtr>
        face_detection_results) {
  DCHECK(face_service_requests_.Contains(resolver));
  face_service_requests_.erase(resolver);

  HeapVector<Member<DetectedFace>> detected_faces;
  for (const auto& face : face_detection_results) {
    HeapVector<Landmark> landmarks;
    for (const auto& landmark : face->landmarks) {
      HeapVector<Point2D> locations;
      for (const auto& location : landmark->locations) {
        Point2D web_location;
        web_location.setX(location.x);
        web_location.setY(location.y);
        locations.push_back(web_location);
      }

      Landmark web_landmark;
      web_landmark.setLocations(locations);
      web_landmark.setType(mojo::ConvertTo<String>(landmark->type));
      landmarks.push_back(web_landmark);
    }

    detected_faces.push_back(DetectedFace::Create(
        DOMRect::Create(face->bounding_box.x, face->bounding_box.y,
                        face->bounding_box.width, face->bounding_box.height),
        landmarks));
  }

  resolver->Resolve(detected_faces);
}

void FaceDetector::OnFaceServiceConnectionError() {
  for (const auto& request : face_service_requests_) {
    request->Reject(DOMException::Create(kNotSupportedError,
                                         "Face Detection not implemented."));
  }
  face_service_requests_.clear();
  face_service_.reset();
}

void FaceDetector::Trace(blink::Visitor* visitor) {
  ShapeDetector::Trace(visitor);
  visitor->Trace(face_service_requests_);
}

}  // namespace blink
