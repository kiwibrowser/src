// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/shapedetection/detected_face.h"

#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/modules/shapedetection/landmark.h"

namespace blink {

DetectedFace* DetectedFace::Create() {
  return new DetectedFace(DOMRect::Create());
}

DetectedFace* DetectedFace::Create(DOMRect* bounding_box) {
  return new DetectedFace(bounding_box);
}

DetectedFace* DetectedFace::Create(DOMRect* bounding_box,
                                   const HeapVector<Landmark>& landmarks) {
  return new DetectedFace(bounding_box, landmarks);
}

DOMRect* DetectedFace::boundingBox() const {
  return bounding_box_.Get();
}

const HeapVector<Landmark>& DetectedFace::landmarks() const {
  return landmarks_;
}

DetectedFace::DetectedFace(DOMRect* bounding_box)
    : bounding_box_(bounding_box) {}

DetectedFace::DetectedFace(DOMRect* bounding_box,
                           const HeapVector<Landmark>& landmarks)
    : bounding_box_(bounding_box), landmarks_(landmarks) {}

void DetectedFace::Trace(blink::Visitor* visitor) {
  visitor->Trace(bounding_box_);
  visitor->Trace(landmarks_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
