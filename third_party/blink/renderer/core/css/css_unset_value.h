// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_UNSET_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_UNSET_VALUE_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/css/css_value.h"

namespace blink {

class CSSValuePool;

namespace cssvalue {

class CSSUnsetValue : public CSSValue {
 public:
  static CSSUnsetValue* Create();

  String CustomCSSText() const;

  bool Equals(const CSSUnsetValue&) const { return true; }

  void TraceAfterDispatch(blink::Visitor* visitor) {
    CSSValue::TraceAfterDispatch(visitor);
  }

 private:
  friend class ::blink::CSSValuePool;

  CSSUnsetValue() : CSSValue(kUnsetClass) {}
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSUnsetValue, IsUnsetValue());

}  // namespace cssvalue
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_UNSET_VALUE_H_
