/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2008, 2009, 2010, 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2011 Motorola Mobility. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SERIALIZERS_STYLED_MARKUP_ACCUMULATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SERIALIZERS_STYLED_MARKUP_ACCUMULATOR_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/editing/editing_style.h"
#include "third_party/blink/renderer/core/editing/serializers/markup_formatter.h"
#include "third_party/blink/renderer/core/editing/serializers/text_offset.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class Document;
class CSSPropertyValueSet;
class Text;

class StyledMarkupAccumulator final {
  STACK_ALLOCATED();

 public:
  StyledMarkupAccumulator(EAbsoluteURLs,
                          const TextOffset& start,
                          const TextOffset& end,
                          Document*,
                          EAnnotateForInterchange,
                          ConvertBlocksToInlines);

  void AppendEndTag(const Element&);
  void AppendInterchangeNewline();

  void AppendText(Text&);
  void AppendTextWithInlineStyle(Text&, EditingStyle*);

  void WrapWithStyleNode(CSSPropertyValueSet*);
  String TakeResults();

  void PushMarkup(const String&);

  void AppendElement(const Element&);
  void AppendElement(StringBuilder&, const Element&);
  void AppendElementWithInlineStyle(const Element&, EditingStyle*);
  void AppendElementWithInlineStyle(StringBuilder&,
                                    const Element&,
                                    EditingStyle*);
  void AppendStartMarkup(Node&);

  bool ShouldAnnotate() const;
  bool ShouldConvertBlocksToInlines() const {
    return convert_blocks_to_inlines_ == ConvertBlocksToInlines::kConvert;
  }

 private:
  String RenderedText(Text&);
  String StringValueForRange(const Text&);

  void AppendEndMarkup(StringBuilder&, const Element&);

  MarkupFormatter formatter_;
  const TextOffset start_;
  const TextOffset end_;
  const Member<Document> document_;
  const EAnnotateForInterchange should_annotate_;
  StringBuilder result_;
  Vector<String> reversed_preceding_markup_;
  const ConvertBlocksToInlines convert_blocks_to_inlines_;

  DISALLOW_COPY_AND_ASSIGN(StyledMarkupAccumulator);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SERIALIZERS_STYLED_MARKUP_ACCUMULATOR_H_
