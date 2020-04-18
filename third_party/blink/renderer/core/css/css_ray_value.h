// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_RAY_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_RAY_VALUE_H_

#include "third_party/blink/renderer/core/css/css_value.h"

namespace blink {

class CSSIdentifierValue;
class CSSPrimitiveValue;

namespace cssvalue {

class CSSRayValue : public CSSValue {
 public:
  static CSSRayValue* Create(const CSSPrimitiveValue& angle,
                             const CSSIdentifierValue& size,
                             const CSSIdentifierValue* contain);

  const CSSPrimitiveValue& Angle() const { return *angle_; }
  const CSSIdentifierValue& Size() const { return *size_; }
  const CSSIdentifierValue* Contain() const { return contain_.Get(); }

  String CustomCSSText() const;

  bool Equals(const CSSRayValue&) const;

  void TraceAfterDispatch(blink::Visitor*);

 private:
  CSSRayValue(const CSSPrimitiveValue& angle,
              const CSSIdentifierValue& size,
              const CSSIdentifierValue* contain);

  Member<const CSSPrimitiveValue> angle_;
  Member<const CSSIdentifierValue> size_;
  Member<const CSSIdentifierValue> contain_;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSRayValue, IsRayValue());

}  // namespace cssvalue
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_RAY_VALUE_H_
