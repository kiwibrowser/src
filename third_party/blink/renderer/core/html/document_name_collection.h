// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_DOCUMENT_NAME_COLLECTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_DOCUMENT_NAME_COLLECTION_H_

#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html/html_name_collection.h"

namespace blink {

class DocumentNameCollection final : public HTMLNameCollection {
 public:
  static DocumentNameCollection* Create(ContainerNode& document,
                                        CollectionType type,
                                        const AtomicString& name) {
    DCHECK_EQ(type, kDocumentNamedItems);
    return new DocumentNameCollection(document, name);
  }

  HTMLElement* Item(unsigned offset) const {
    return ToHTMLElement(HTMLNameCollection::item(offset));
  }

  bool ElementMatches(const HTMLElement&) const;

 private:
  DocumentNameCollection(ContainerNode& document, const AtomicString& name);
};

DEFINE_TYPE_CASTS(DocumentNameCollection,
                  LiveNodeListBase,
                  collection,
                  collection->GetType() == kDocumentNamedItems,
                  collection.GetType() == kDocumentNamedItems);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_DOCUMENT_NAME_COLLECTION_H_
