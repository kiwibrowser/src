// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_CONTENT_DISTRIBUTION_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_CONTENT_DISTRIBUTION_VALUE_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/core/css/css_value_pair.h"

namespace blink {
namespace cssvalue {

class CSSContentDistributionValue : public CSSValue {
 public:
  static CSSContentDistributionValue* Create(CSSValueID distribution,
                                             CSSValueID position,
                                             CSSValueID overflow) {
    return new CSSContentDistributionValue(distribution, position, overflow);
  }
  ~CSSContentDistributionValue();

  CSSValueID Distribution() const { return distribution_; }

  CSSValueID Position() const { return position_; }

  CSSValueID Overflow() const { return overflow_; }

  String CustomCSSText() const;

  bool Equals(const CSSContentDistributionValue&) const;

  void TraceAfterDispatch(blink::Visitor* visitor) {
    CSSValue::TraceAfterDispatch(visitor);
  }

 private:
  CSSContentDistributionValue(CSSValueID distribution,
                              CSSValueID position,
                              CSSValueID overflow);

  CSSValueID distribution_;
  CSSValueID position_;
  CSSValueID overflow_;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSContentDistributionValue,
                            IsContentDistributionValue());

}  // namespace cssvalue
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_CONTENT_DISTRIBUTION_VALUE_H_
