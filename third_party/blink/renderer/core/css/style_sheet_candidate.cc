/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/css/style_sheet_candidate.h"

#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/processing_instruction.h"
#include "third_party/blink/renderer/core/html/html_link_element.h"
#include "third_party/blink/renderer/core/html/html_style_element.h"
#include "third_party/blink/renderer/core/html/imports/html_import.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/svg/svg_style_element.h"

namespace blink {

using namespace HTMLNames;

AtomicString StyleSheetCandidate::Title() const {
  return IsElement() ? ToElement(GetNode()).FastGetAttribute(titleAttr)
                     : g_null_atom;
}

bool StyleSheetCandidate::IsXSL() const {
  return !GetNode().GetDocument().IsHTMLDocument() && type_ == kPi &&
         ToProcessingInstruction(GetNode()).IsXSL();
}

bool StyleSheetCandidate::IsImport() const {
  return type_ == kHTMLLink && ToHTMLLinkElement(GetNode()).IsImport();
}

bool StyleSheetCandidate::IsCSSStyle() const {
  return type_ == kHTMLStyle || type_ == kSVGStyle;
}

Document* StyleSheetCandidate::ImportedDocument() const {
  DCHECK(IsImport());
  return ToHTMLLinkElement(GetNode()).import();
}

bool StyleSheetCandidate::IsEnabledViaScript() const {
  return IsHTMLLink() && ToHTMLLinkElement(GetNode()).IsEnabledViaScript();
}

bool StyleSheetCandidate::IsEnabledAndLoading() const {
  return IsHTMLLink() && !ToHTMLLinkElement(GetNode()).IsDisabled() &&
         ToHTMLLinkElement(GetNode()).StyleSheetIsLoading();
}

bool StyleSheetCandidate::CanBeActivated(
    const String& current_preferrable_name) const {
  StyleSheet* sheet = this->Sheet();
  if (!sheet || sheet->disabled() || !sheet->IsCSSStyleSheet())
    return false;
  return ToCSSStyleSheet(Sheet())->CanBeActivated(current_preferrable_name);
}

StyleSheetCandidate::Type StyleSheetCandidate::TypeOf(Node& node) {
  if (node.getNodeType() == Node::kProcessingInstructionNode)
    return kPi;

  if (node.IsHTMLElement()) {
    if (IsHTMLLinkElement(node))
      return kHTMLLink;
    if (IsHTMLStyleElement(node))
      return kHTMLStyle;

    NOTREACHED();
    return kInvalid;
  }

  if (IsSVGStyleElement(node))
    return kSVGStyle;

  NOTREACHED();
  return kInvalid;
}

StyleSheet* StyleSheetCandidate::Sheet() const {
  switch (type_) {
    case kHTMLLink:
      return ToHTMLLinkElement(GetNode()).sheet();
    case kHTMLStyle:
      return ToHTMLStyleElement(GetNode()).sheet();
    case kSVGStyle:
      return ToSVGStyleElement(GetNode()).sheet();
    case kPi:
      return ToProcessingInstruction(GetNode()).sheet();
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace blink
