// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_ADOPTED_CALLBACK_REACTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_ADOPTED_CALLBACK_REACTION_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_reaction.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Document;

class CORE_EXPORT CustomElementAdoptedCallbackReaction final
    : public CustomElementReaction {
 public:
  CustomElementAdoptedCallbackReaction(CustomElementDefinition*,
                                       Document* old_owner,
                                       Document* new_owner);

  void Trace(blink::Visitor*) override;

 private:
  void Invoke(Element*) override;

  Member<Document> old_owner_;
  Member<Document> new_owner_;

  DISALLOW_COPY_AND_ASSIGN(CustomElementAdoptedCallbackReaction);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_ADOPTED_CALLBACK_REACTION_H_
