/*
 * Copyright (C) 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/html/html_view_source_document.h"

#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/html/html_anchor_element.h"
#include "third_party/blink/renderer/core/html/html_base_element.h"
#include "third_party/blink/renderer/core/html/html_body_element.h"
#include "third_party/blink/renderer/core/html/html_br_element.h"
#include "third_party/blink/renderer/core/html/html_div_element.h"
#include "third_party/blink/renderer/core/html/html_head_element.h"
#include "third_party/blink/renderer/core/html/html_html_element.h"
#include "third_party/blink/renderer/core/html/html_span_element.h"
#include "third_party/blink/renderer/core/html/html_table_cell_element.h"
#include "third_party/blink/renderer/core/html/html_table_element.h"
#include "third_party/blink/renderer/core/html/html_table_row_element.h"
#include "third_party/blink/renderer/core/html/html_table_section_element.h"
#include "third_party/blink/renderer/core/html/parser/html_view_source_parser.h"

namespace blink {

using namespace HTMLNames;

namespace {

const char kXSSDetected[] = "Token contains a reflected XSS vector";

}  // namespace

HTMLViewSourceDocument::HTMLViewSourceDocument(const DocumentInit& initializer,
                                               const String& mime_type)
    : HTMLDocument(initializer), type_(mime_type) {
  SetIsViewSource(true);

  // FIXME: Why do view-source pages need to load in quirks mode?
  SetCompatibilityMode(kQuirksMode);
  LockCompatibilityMode();
}

DocumentParser* HTMLViewSourceDocument::CreateParser() {
  return HTMLViewSourceParser::Create(*this, type_);
}

void HTMLViewSourceDocument::CreateContainingTable() {
  HTMLHtmlElement* html = HTMLHtmlElement::Create(*this);
  ParserAppendChild(html);
  HTMLHeadElement* head = HTMLHeadElement::Create(*this);
  html->ParserAppendChild(head);
  HTMLBodyElement* body = HTMLBodyElement::Create(*this);
  html->ParserAppendChild(body);

  // Create a line gutter div that can be used to make sure the gutter extends
  // down the height of the whole document.
  HTMLDivElement* div = HTMLDivElement::Create(*this);
  div->setAttribute(classAttr, "line-gutter-backdrop");
  body->ParserAppendChild(div);

  HTMLTableElement* table = HTMLTableElement::Create(*this);
  body->ParserAppendChild(table);
  tbody_ = HTMLTableSectionElement::Create(tbodyTag, *this);
  table->ParserAppendChild(tbody_);
  current_ = tbody_;
  line_number_ = 0;
}

void HTMLViewSourceDocument::AddSource(const String& source,
                                       HTMLToken& token,
                                       SourceAnnotation annotation) {
  if (!current_)
    CreateContainingTable();

  switch (token.GetType()) {
    case HTMLToken::kUninitialized:
      NOTREACHED();
      break;
    case HTMLToken::DOCTYPE:
      ProcessDoctypeToken(source, token);
      break;
    case HTMLToken::kEndOfFile:
      ProcessEndOfFileToken(source, token);
      break;
    case HTMLToken::kStartTag:
    case HTMLToken::kEndTag:
      ProcessTagToken(source, token, annotation);
      break;
    case HTMLToken::kComment:
      ProcessCommentToken(source, token);
      break;
    case HTMLToken::kCharacter:
      ProcessCharacterToken(source, token, annotation);
      break;
  }
}

void HTMLViewSourceDocument::ProcessDoctypeToken(const String& source,
                                                 HTMLToken&) {
  current_ = AddSpanWithClassName("html-doctype");
  AddText(source, "html-doctype");
  current_ = td_;
}

void HTMLViewSourceDocument::ProcessEndOfFileToken(const String& source,
                                                   HTMLToken&) {
  current_ = AddSpanWithClassName("html-end-of-file");
  AddText(source, "html-end-of-file");
  current_ = td_;
}

void HTMLViewSourceDocument::ProcessTagToken(const String& source,
                                             HTMLToken& token,
                                             SourceAnnotation annotation) {
  MaybeAddSpanForAnnotation(annotation);
  current_ = AddSpanWithClassName("html-tag");

  AtomicString tag_name(token.GetName());

  unsigned index = 0;
  HTMLToken::AttributeList::const_iterator iter = token.Attributes().begin();
  while (index < source.length()) {
    if (iter == token.Attributes().end()) {
      // We want to show the remaining characters in the token.
      index = AddRange(source, index, source.length(), g_empty_atom);
      DCHECK_EQ(index, source.length());
      break;
    }

    AtomicString name(iter->GetName());
    AtomicString value(iter->Value8BitIfNecessary());

    index =
        AddRange(source, index, iter->NameRange().start - token.StartIndex(),
                 g_empty_atom);
    index = AddRange(source, index, iter->NameRange().end - token.StartIndex(),
                     "html-attribute-name");

    if (tag_name == baseTag && name == hrefAttr)
      AddBase(value);

    index =
        AddRange(source, index, iter->ValueRange().start - token.StartIndex(),
                 g_empty_atom);

    if (name == srcsetAttr) {
      index =
          AddSrcset(source, index, iter->ValueRange().end - token.StartIndex());
    } else {
      bool is_link = name == srcAttr || name == hrefAttr;
      index =
          AddRange(source, index, iter->ValueRange().end - token.StartIndex(),
                   "html-attribute-value", is_link, tag_name == aTag, value);
    }

    ++iter;
  }
  current_ = td_;
}

void HTMLViewSourceDocument::ProcessCommentToken(const String& source,
                                                 HTMLToken&) {
  current_ = AddSpanWithClassName("html-comment");
  AddText(source, "html-comment");
  current_ = td_;
}

void HTMLViewSourceDocument::ProcessCharacterToken(
    const String& source,
    HTMLToken&,
    SourceAnnotation annotation) {
  AddText(source, "", annotation);
}

Element* HTMLViewSourceDocument::AddSpanWithClassName(
    const AtomicString& class_name) {
  if (current_ == tbody_) {
    AddLine(class_name);
    return current_;
  }

  HTMLSpanElement* span = HTMLSpanElement::Create(*this);
  span->setAttribute(classAttr, class_name);
  current_->ParserAppendChild(span);
  return span;
}

void HTMLViewSourceDocument::AddLine(const AtomicString& class_name) {
  // Create a table row.
  HTMLTableRowElement* trow = HTMLTableRowElement::Create(*this);
  tbody_->ParserAppendChild(trow);

  // Create a cell that will hold the line number (it is generated in the
  // stylesheet using counters).
  HTMLTableCellElement* td = HTMLTableCellElement::Create(tdTag, *this);
  td->setAttribute(classAttr, "line-number");
  td->SetIntegralAttribute(valueAttr, ++line_number_);
  trow->ParserAppendChild(td);

  // Create a second cell for the line contents
  td = HTMLTableCellElement::Create(tdTag, *this);
  td->setAttribute(classAttr, "line-content");
  trow->ParserAppendChild(td);
  current_ = td_ = td;

  // Open up the needed spans.
  if (!class_name.IsEmpty()) {
    if (class_name == "html-attribute-name" ||
        class_name == "html-attribute-value")
      current_ = AddSpanWithClassName("html-tag");
    current_ = AddSpanWithClassName(class_name);
  }
}

void HTMLViewSourceDocument::FinishLine() {
  if (!current_->HasChildren()) {
    HTMLBRElement* br = HTMLBRElement::Create(*this);
    current_->ParserAppendChild(br);
  }
  current_ = tbody_;
}

void HTMLViewSourceDocument::AddText(const String& text,
                                     const AtomicString& class_name,
                                     SourceAnnotation annotation) {
  if (text.IsEmpty())
    return;

  // Add in the content, splitting on newlines.
  Vector<String> lines;
  text.Split('\n', true, lines);
  unsigned size = lines.size();
  for (unsigned i = 0; i < size; i++) {
    String substring = lines[i];
    if (current_ == tbody_)
      AddLine(class_name);
    if (substring.IsEmpty()) {
      if (i == size - 1)
        break;
      FinishLine();
      continue;
    }
    Element* old_element = current_;
    MaybeAddSpanForAnnotation(annotation);
    current_->ParserAppendChild(Text::Create(*this, substring));
    current_ = old_element;
    if (i < size - 1)
      FinishLine();
  }
}

int HTMLViewSourceDocument::AddRange(const String& source,
                                     int start,
                                     int end,
                                     const AtomicString& class_name,
                                     bool is_link,
                                     bool is_anchor,
                                     const AtomicString& link) {
  DCHECK_LE(start, end);
  if (start == end)
    return start;

  String text = source.Substring(start, end - start);
  if (!class_name.IsEmpty()) {
    if (is_link)
      current_ = AddLink(link, is_anchor);
    else
      current_ = AddSpanWithClassName(class_name);
  }
  AddText(text, class_name);
  if (!class_name.IsEmpty() && current_ != tbody_)
    current_ = ToElement(current_->parentNode());
  return end;
}

Element* HTMLViewSourceDocument::AddBase(const AtomicString& href) {
  HTMLBaseElement* base = HTMLBaseElement::Create(*this);
  base->setAttribute(hrefAttr, href);
  current_->ParserAppendChild(base);
  return base;
}

Element* HTMLViewSourceDocument::AddLink(const AtomicString& url,
                                         bool is_anchor) {
  if (current_ == tbody_)
    AddLine("html-tag");

  // Now create a link for the attribute value instead of a span.
  HTMLAnchorElement* anchor = HTMLAnchorElement::Create(*this);
  const char* class_value;
  if (is_anchor)
    class_value = "html-attribute-value html-external-link";
  else
    class_value = "html-attribute-value html-resource-link";
  anchor->setAttribute(classAttr, class_value);
  anchor->setAttribute(targetAttr, "_blank");
  anchor->setAttribute(hrefAttr, url);
  anchor->setAttribute(relAttr, "noreferrer noopener");
  // Disallow JavaScript hrefs. https://crbug.com/808407
  if (anchor->Url().ProtocolIsJavaScript())
    anchor->setAttribute(hrefAttr, "about:blank");
  current_->ParserAppendChild(anchor);
  return anchor;
}

int HTMLViewSourceDocument::AddSrcset(const String& source,
                                      int start,
                                      int end) {
  String srcset = source.Substring(start, end - start);
  Vector<String> srclist;
  srcset.Split(',', true, srclist);
  unsigned size = srclist.size();
  for (unsigned i = 0; i < size; i++) {
    Vector<String> tmp;
    srclist[i].Split(' ', tmp);
    if (tmp.size() > 0) {
      AtomicString link(tmp[0]);
      current_ = AddLink(link, false);
      AddText(srclist[i], "html-attribute-value");
      current_ = ToElement(current_->parentNode());
    } else {
      AddText(srclist[i], "html-attribute-value");
    }
    if (i + 1 < size)
      AddText(",", "html-attribute-value");
  }
  return end;
}

void HTMLViewSourceDocument::MaybeAddSpanForAnnotation(
    SourceAnnotation annotation) {
  if (annotation == kAnnotateSourceAsXSS) {
    current_ = AddSpanWithClassName("highlight");
    current_->setAttribute(titleAttr, kXSSDetected);
  }
}

void HTMLViewSourceDocument::Trace(blink::Visitor* visitor) {
  visitor->Trace(current_);
  visitor->Trace(tbody_);
  visitor->Trace(td_);
  HTMLDocument::Trace(visitor);
}

}  // namespace blink
