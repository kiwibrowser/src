// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/shapedetection/detected_barcode.h"

#include "third_party/blink/renderer/core/geometry/dom_rect.h"

namespace blink {

DetectedBarcode* DetectedBarcode::Create() {
  HeapVector<Point2D> empty_list;
  return new DetectedBarcode(g_empty_string, DOMRect::Create(0, 0, 0, 0),
                             empty_list);
}

DetectedBarcode* DetectedBarcode::Create(String raw_value,
                                         DOMRect* bounding_box,
                                         HeapVector<Point2D> corner_points) {
  return new DetectedBarcode(raw_value, bounding_box, corner_points);
}

const String& DetectedBarcode::rawValue() const {
  return raw_value_;
}

DOMRect* DetectedBarcode::boundingBox() const {
  return bounding_box_.Get();
}

const HeapVector<Point2D>& DetectedBarcode::cornerPoints() const {
  return corner_points_;
}

DetectedBarcode::DetectedBarcode(String raw_value,
                                 DOMRect* bounding_box,
                                 HeapVector<Point2D> corner_points)
    : raw_value_(raw_value),
      bounding_box_(bounding_box),
      corner_points_(corner_points) {}

void DetectedBarcode::Trace(blink::Visitor* visitor) {
  visitor->Trace(bounding_box_);
  visitor->Trace(corner_points_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
