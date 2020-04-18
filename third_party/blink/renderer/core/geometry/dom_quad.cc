// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/geometry/dom_quad.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_object_builder.h"
#include "third_party/blink/renderer/core/geometry/dom_point.h"
#include "third_party/blink/renderer/core/geometry/dom_quad_init.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/geometry/dom_rect_init.h"

namespace blink {

DOMQuad* DOMQuad::Create(const DOMPointInit& p1,
                         const DOMPointInit& p2,
                         const DOMPointInit& p3,
                         const DOMPointInit& p4) {
  return new DOMQuad(p1, p2, p3, p4);
}

DOMQuad* DOMQuad::fromRect(const DOMRectInit& other) {
  return new DOMQuad(other.x(), other.y(), other.width(), other.height());
}

DOMQuad* DOMQuad::fromQuad(const DOMQuadInit& other) {
  return new DOMQuad(other.hasP1() ? other.p1() : DOMPointInit(),
                     other.hasP2() ? other.p2() : DOMPointInit(),
                     other.hasP3() ? other.p3() : DOMPointInit(),
                     other.hasP3() ? other.p4() : DOMPointInit());
}

DOMRect* DOMQuad::getBounds() {
  return DOMRect::Create(left_, top_, right_ - left_, bottom_ - top_);
}

void DOMQuad::CalculateBounds() {
  left_ = std::min(p1()->x(), p2()->x());
  left_ = std::min(left_, p3()->x());
  left_ = std::min(left_, p4()->x());
  top_ = std::min(p1()->y(), p2()->y());
  top_ = std::min(top_, p3()->y());
  top_ = std::min(top_, p4()->y());
  right_ = std::max(p1()->x(), p2()->x());
  right_ = std::max(right_, p3()->x());
  right_ = std::max(right_, p4()->x());
  bottom_ = std::max(p1()->y(), p2()->y());
  bottom_ = std::max(bottom_, p3()->y());
  bottom_ = std::max(bottom_, p4()->y());
}

DOMQuad::DOMQuad(const DOMPointInit& p1,
                 const DOMPointInit& p2,
                 const DOMPointInit& p3,
                 const DOMPointInit& p4)
    : p1_(DOMPoint::fromPoint(p1)),
      p2_(DOMPoint::fromPoint(p2)),
      p3_(DOMPoint::fromPoint(p3)),
      p4_(DOMPoint::fromPoint(p4)) {
  CalculateBounds();
}

DOMQuad::DOMQuad(double x, double y, double width, double height)
    : p1_(DOMPoint::Create(x, y, 0, 1)),
      p2_(DOMPoint::Create(x + width, y, 0, 1)),
      p3_(DOMPoint::Create(x + width, y + height, 0, 1)),
      p4_(DOMPoint::Create(x, y + height, 0, 1)) {
  CalculateBounds();
}

ScriptValue DOMQuad::toJSONForBinding(ScriptState* script_state) const {
  V8ObjectBuilder result(script_state);
  result.Add("p1", p1());
  result.Add("p2", p2());
  result.Add("p3", p3());
  result.Add("p4", p4());
  return result.GetScriptValue();
}

}  // namespace blink
