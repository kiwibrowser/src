// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_VARIABLE_REFERENCE_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_VARIABLE_REFERENCE_VALUE_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/core/css/css_variable_data.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"

namespace blink {

class CSSVariableReferenceValue : public CSSValue {
 public:
  static CSSVariableReferenceValue* Create(
      scoped_refptr<CSSVariableData> data) {
    return new CSSVariableReferenceValue(std::move(data));
  }
  static CSSVariableReferenceValue* Create(scoped_refptr<CSSVariableData> data,
                                           const CSSParserContext& context) {
    return new CSSVariableReferenceValue(std::move(data), context);
  }

  CSSVariableData* VariableDataValue() const { return data_.get(); }
  const CSSParserContext* ParserContext() const {
    DCHECK(parser_context_);
    return parser_context_.Get();
  }

  bool Equals(const CSSVariableReferenceValue& other) const {
    return data_ == other.data_;
  }
  String CustomCSSText() const;

  void TraceAfterDispatch(blink::Visitor*);

 private:
  CSSVariableReferenceValue(scoped_refptr<CSSVariableData> data)
      : CSSValue(kVariableReferenceClass),
        data_(std::move(data)),
        parser_context_(nullptr) {}

  CSSVariableReferenceValue(scoped_refptr<CSSVariableData> data,
                            const CSSParserContext& context)
      : CSSValue(kVariableReferenceClass),
        data_(std::move(data)),
        parser_context_(context) {
  }

  scoped_refptr<CSSVariableData> data_;
  Member<const CSSParserContext> parser_context_;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSVariableReferenceValue,
                            IsVariableReferenceValue());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_VARIABLE_REFERENCE_VALUE_H_
