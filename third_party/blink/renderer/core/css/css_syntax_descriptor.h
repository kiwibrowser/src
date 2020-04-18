// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_SYNTAX_DESCRIPTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_SYNTAX_DESCRIPTOR_H_

#include "third_party/blink/renderer/core/css/parser/css_parser_token_range.h"

namespace blink {

class CSSParserContext;
class CSSValue;

enum class CSSSyntaxType {
  kTokenStream,
  kIdent,
  kLength,
  kNumber,
  kPercentage,
  kLengthPercentage,
  kColor,
  kImage,
  kUrl,
  kInteger,
  kAngle,
  kTime,
  kResolution,
  kTransformList,
  kCustomIdent,
};

struct CSSSyntaxComponent {
  CSSSyntaxComponent(CSSSyntaxType type, const String& string, bool repeatable)
      : type_(type), string_(string), repeatable_(repeatable) {}

  bool operator==(const CSSSyntaxComponent& a) const {
    return type_ == a.type_ && string_ == a.string_ &&
           repeatable_ == a.repeatable_;
  }

  CSSSyntaxType type_;
  String string_;  // Only used when type_ is CSSSyntaxType::kIdent
  bool repeatable_;
};

class CORE_EXPORT CSSSyntaxDescriptor {
 public:
  explicit CSSSyntaxDescriptor(const String& syntax);

  const CSSValue* Parse(CSSParserTokenRange,
                        const CSSParserContext*,
                        bool is_animation_tainted) const;
  bool IsValid() const { return !syntax_components_.IsEmpty(); }
  bool IsTokenStream() const {
    return syntax_components_.size() == 1 &&
           syntax_components_[0].type_ == CSSSyntaxType::kTokenStream;
  }
  const Vector<CSSSyntaxComponent>& Components() const {
    return syntax_components_;
  }
  bool operator==(const CSSSyntaxDescriptor& a) const {
    return Components() == a.Components();
  }
  bool operator!=(const CSSSyntaxDescriptor& a) const {
    return Components() != a.Components();
  }

 private:
  Vector<CSSSyntaxComponent> syntax_components_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_SYNTAX_DESCRIPTOR_H_
