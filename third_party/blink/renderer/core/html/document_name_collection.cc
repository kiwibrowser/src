// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/document_name_collection.h"

#include "third_party/blink/renderer/core/html/html_embed_element.h"
#include "third_party/blink/renderer/core/html/html_object_element.h"

namespace blink {

DocumentNameCollection::DocumentNameCollection(ContainerNode& document,
                                               const AtomicString& name)
    : HTMLNameCollection(document, kDocumentNamedItems, name) {}

// https://html.spec.whatwg.org/multipage/dom.html#dom-document-nameditem-filter
bool DocumentNameCollection::ElementMatches(const HTMLElement& element) const {
  // Match images, forms, embeds, objects and iframes by name,
  // object by id, and images by id but only if they have
  // a name attribute (this very strange rule matches IE)
  if (IsHTMLFormElement(element) || IsHTMLIFrameElement(element) ||
      (IsHTMLEmbedElement(element) && ToHTMLEmbedElement(element).IsExposed()))
    return element.GetNameAttribute() == name_;
  if (IsHTMLObjectElement(element) && ToHTMLObjectElement(element).IsExposed())
    return element.GetNameAttribute() == name_ ||
           element.GetIdAttribute() == name_;
  if (IsHTMLImageElement(element)) {
    const AtomicString& name_value = element.GetNameAttribute();
    return name_value == name_ ||
           (element.GetIdAttribute() == name_ && !name_value.IsEmpty());
  }
  return false;
}

}  // namespace blink
