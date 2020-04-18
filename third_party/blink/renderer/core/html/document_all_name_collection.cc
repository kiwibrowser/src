// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/document_all_name_collection.h"
#include "third_party/blink/renderer/core/html/html_element.h"

namespace blink {

DocumentAllNameCollection::DocumentAllNameCollection(ContainerNode& document,
                                                     const AtomicString& name)
    : HTMLNameCollection(document, kDocumentAllNamedItems, name) {}

bool DocumentAllNameCollection::ElementMatches(const Element& element) const {
  // https://html.spec.whatwg.org/multipage/common-dom-interfaces.html#all-named-elements
  // Match below type of elements by name but any type of element by id.
  if (element.HasTagName(HTMLNames::aTag) ||
      element.HasTagName(HTMLNames::buttonTag) ||
      element.HasTagName(HTMLNames::embedTag) ||
      element.HasTagName(HTMLNames::formTag) ||
      element.HasTagName(HTMLNames::frameTag) ||
      element.HasTagName(HTMLNames::framesetTag) ||
      element.HasTagName(HTMLNames::iframeTag) ||
      element.HasTagName(HTMLNames::imgTag) ||
      element.HasTagName(HTMLNames::inputTag) ||
      element.HasTagName(HTMLNames::mapTag) ||
      element.HasTagName(HTMLNames::metaTag) ||
      element.HasTagName(HTMLNames::objectTag) ||
      element.HasTagName(HTMLNames::selectTag) ||
      element.HasTagName(HTMLNames::textareaTag)) {
    if (element.GetNameAttribute() == name_)
      return true;
  }

  return element.GetIdAttribute() == name_;
}

}  // namespace blink
