/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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

#include "third_party/blink/renderer/core/html/html_template_element.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/document_fragment.h"
#include "third_party/blink/renderer/core/dom/template_content_document_fragment.h"

namespace blink {

using namespace HTMLNames;

inline HTMLTemplateElement::HTMLTemplateElement(Document& document)
    : HTMLElement(templateTag, document) {}

DEFINE_NODE_FACTORY(HTMLTemplateElement)

HTMLTemplateElement::~HTMLTemplateElement() = default;

DocumentFragment* HTMLTemplateElement::content() const {
  if (!content_)
    content_ = TemplateContentDocumentFragment::Create(
        GetDocument().EnsureTemplateDocument(),
        const_cast<HTMLTemplateElement*>(this));

  return content_.Get();
}

// https://html.spec.whatwg.org/multipage/scripting.html#the-template-element:concept-node-clone-ext
void HTMLTemplateElement::CloneNonAttributePropertiesFrom(
    const Element& source,
    CloneChildrenFlag flag) {
  if (flag == CloneChildrenFlag::kSkip)
    return;
  if (ToHTMLTemplateElement(source).content_)
    content()->CloneChildNodesFrom(*ToHTMLTemplateElement(source).content());
}

void HTMLTemplateElement::DidMoveToNewDocument(Document& old_document) {
  HTMLElement::DidMoveToNewDocument(old_document);
  if (!content_)
    return;
  GetDocument().EnsureTemplateDocument().AdoptIfNeeded(*content_);
}

void HTMLTemplateElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(content_);
  HTMLElement::Trace(visitor);
}

void HTMLTemplateElement::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(content_);
  HTMLElement::TraceWrappers(visitor);
}

}  // namespace blink
