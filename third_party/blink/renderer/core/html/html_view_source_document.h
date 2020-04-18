/*
 * Copyright (C) 2006, 2008, 2009 Apple Inc. All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_VIEW_SOURCE_DOCUMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_VIEW_SOURCE_DOCUMENT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/html/html_document.h"

namespace blink {

class HTMLTableCellElement;
class HTMLTableSectionElement;
class HTMLToken;

class CORE_EXPORT HTMLViewSourceDocument final : public HTMLDocument {
 public:
  enum SourceAnnotation { kAnnotateSourceAsSafe, kAnnotateSourceAsXSS };

  static HTMLViewSourceDocument* Create(const DocumentInit& initializer,
                                        const String& mime_type) {
    return new HTMLViewSourceDocument(initializer, mime_type);
  }

  void AddSource(const String&, HTMLToken&, SourceAnnotation);

  void Trace(blink::Visitor*) override;

 private:
  HTMLViewSourceDocument(const DocumentInit&, const String& mime_type);

  DocumentParser* CreateParser() override;

  void ProcessDoctypeToken(const String& source, HTMLToken&);
  void ProcessEndOfFileToken(const String& source, HTMLToken&);
  void ProcessTagToken(const String& source, HTMLToken&, SourceAnnotation);
  void ProcessCommentToken(const String& source, HTMLToken&);
  void ProcessCharacterToken(const String& source,
                             HTMLToken&,
                             SourceAnnotation);

  void CreateContainingTable();
  Element* AddSpanWithClassName(const AtomicString&);
  void AddLine(const AtomicString& class_name);
  void FinishLine();
  void AddText(const String& text,
               const AtomicString& class_name,
               SourceAnnotation = kAnnotateSourceAsSafe);
  int AddRange(const String& source,
               int start,
               int end,
               const AtomicString& class_name,
               bool is_link = false,
               bool is_anchor = false,
               const AtomicString& link = g_null_atom);
  int AddSrcset(const String& source, int start, int end);
  void MaybeAddSpanForAnnotation(SourceAnnotation);

  Element* AddLink(const AtomicString& url, bool is_anchor);
  Element* AddBase(const AtomicString& href);

  String type_;
  Member<Element> current_;
  Member<HTMLTableSectionElement> tbody_;
  Member<HTMLTableCellElement> td_;
  int line_number_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_HTML_VIEW_SOURCE_DOCUMENT_H_
