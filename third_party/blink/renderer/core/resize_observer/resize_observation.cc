// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/resize_observer/resize_observation.h"

#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observer.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/core/svg/svg_graphics_element.h"

namespace blink {

ResizeObservation::ResizeObservation(Element* target, ResizeObserver* observer)
    : target_(target),
      observer_(observer),
      observation_size_(0, 0),
      element_size_changed_(true) {
  DCHECK(target_);
  observer_->ElementSizeChanged();
}

bool ResizeObservation::ObservationSizeOutOfSync() {
  return element_size_changed_ && observation_size_ != ComputeTargetSize();
}

void ResizeObservation::SetObservationSize(const LayoutSize& observation_size) {
  observation_size_ = observation_size;
  element_size_changed_ = false;
}

size_t ResizeObservation::TargetDepth() {
  unsigned depth = 0;
  for (Element* parent = target_; parent; parent = parent->parentElement())
    ++depth;
  return depth;
}

LayoutSize ResizeObservation::ComputeTargetSize() const {
  if (target_) {
    if (LayoutObject* layout_object = target_->GetLayoutObject()) {
      if (target_->IsSVGElement() &&
          ToSVGElement(target_)->IsSVGGraphicsElement()) {
        SVGGraphicsElement& svg = ToSVGGraphicsElement(*target_);
        return LayoutSize(svg.GetBBox().Size());
      }
      if (layout_object->IsBox())
        return ToLayoutBox(layout_object)->ContentSize();
    }
  }
  return LayoutSize();
}

LayoutPoint ResizeObservation::ComputeTargetLocation() const {
  if (target_ && !target_->IsSVGElement()) {
    if (LayoutBox* layout = target_->GetLayoutBox())
      return LayoutPoint(layout->PaddingLeft(), layout->PaddingTop());
  }
  return LayoutPoint();
}

void ResizeObservation::ElementSizeChanged() {
  element_size_changed_ = true;
  observer_->ElementSizeChanged();
}

void ResizeObservation::Trace(blink::Visitor* visitor) {
  visitor->Trace(target_);
  visitor->Trace(observer_);
}

}  // namespace blink
