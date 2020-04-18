// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/svg/svg_animated_string.h"

namespace blink {

String SVGAnimatedString::baseVal() {
  return SVGAnimatedProperty<SVGString>::baseVal();
}

void SVGAnimatedString::setBaseVal(const String& value,
                                   ExceptionState& exception_state) {
  return SVGAnimatedProperty<SVGString>::setBaseVal(value, exception_state);
}

String SVGAnimatedString::animVal() {
  return SVGAnimatedProperty<SVGString>::animVal();
}

void SVGAnimatedString::Trace(blink::Visitor* visitor) {
  SVGAnimatedProperty<SVGString>::Trace(visitor);
  ScriptWrappable::Trace(visitor);
}

void SVGAnimatedString::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  SVGAnimatedProperty<SVGString>::TraceWrappers(visitor);
  ScriptWrappable::TraceWrappers(visitor);
}

}  // namespace blink
