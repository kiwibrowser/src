// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_content_distribution_value.h"

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {
namespace cssvalue {

CSSContentDistributionValue::CSSContentDistributionValue(
    CSSValueID distribution,
    CSSValueID position,
    CSSValueID overflow)
    : CSSValue(kCSSContentDistributionClass),
      distribution_(distribution),
      position_(position),
      overflow_(overflow) {}

CSSContentDistributionValue::~CSSContentDistributionValue() = default;

String CSSContentDistributionValue::CustomCSSText() const {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();

  if (distribution_ != CSSValueInvalid)
    list->Append(*CSSIdentifierValue::Create(distribution_));
  if (position_ != CSSValueInvalid) {
    if (position_ == CSSValueFirstBaseline ||
        position_ == CSSValueLastBaseline) {
      CSSValueID preference =
          position_ == CSSValueFirstBaseline ? CSSValueFirst : CSSValueLast;
      list->Append(*CSSIdentifierValue::Create(preference));
      list->Append(*CSSIdentifierValue::Create(CSSValueBaseline));
    } else {
      if (overflow_ != CSSValueInvalid)
        list->Append(*CSSIdentifierValue::Create(overflow_));
      list->Append(*CSSIdentifierValue::Create(position_));
    }
  }
  return list->CustomCSSText();
}

bool CSSContentDistributionValue::Equals(
    const CSSContentDistributionValue& other) const {
  return distribution_ == other.distribution_ && position_ == other.position_ &&
         overflow_ == other.overflow_;
}

}  // namespace cssvalue
}  // namespace blink
