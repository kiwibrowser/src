// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_DOCUMENT_ALL_NAME_COLLECTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_DOCUMENT_ALL_NAME_COLLECTION_H_

#include "third_party/blink/renderer/core/html/html_name_collection.h"

namespace blink {

// DocumentAllNameCollection implements document.all namedItem as
// HTMLCollection.
class DocumentAllNameCollection final : public HTMLNameCollection {
 public:
  static DocumentAllNameCollection* Create(ContainerNode& document,
                                           CollectionType type,
                                           const AtomicString& name) {
    DCHECK_EQ(type, kDocumentAllNamedItems);
    return new DocumentAllNameCollection(document, name);
  }

  bool ElementMatches(const Element&) const;

 private:
  DocumentAllNameCollection(ContainerNode& document, const AtomicString& name);
};

DEFINE_TYPE_CASTS(DocumentAllNameCollection,
                  LiveNodeListBase,
                  collection,
                  collection->GetType() == kDocumentAllNamedItems,
                  collection.GetType() == kDocumentAllNamedItems);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_DOCUMENT_ALL_NAME_COLLECTION_H_
