// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/custom/custom_element_adopted_callback_reaction.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_definition.h"

namespace blink {

CustomElementAdoptedCallbackReaction::CustomElementAdoptedCallbackReaction(
    CustomElementDefinition* definition,
    Document* old_owner,
    Document* new_owner)
    : CustomElementReaction(definition),
      old_owner_(old_owner),
      new_owner_(new_owner) {
  DCHECK(definition->HasAdoptedCallback());
}

void CustomElementAdoptedCallbackReaction::Trace(blink::Visitor* visitor) {
  CustomElementReaction::Trace(visitor);
  visitor->Trace(old_owner_);
  visitor->Trace(new_owner_);
}

void CustomElementAdoptedCallbackReaction::Invoke(Element* element) {
  definition_->RunAdoptedCallback(element, old_owner_.Get(), new_owner_.Get());
}

}  // namespace blink
