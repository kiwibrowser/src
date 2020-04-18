// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/custom/custom_element_upgrade_reaction.h"

#include "third_party/blink/renderer/core/html/custom/custom_element_definition.h"

namespace blink {

CustomElementUpgradeReaction::CustomElementUpgradeReaction(
    CustomElementDefinition* definition)
    : CustomElementReaction(definition) {}

void CustomElementUpgradeReaction::Invoke(Element* element) {
  // Don't call upgrade() if it's already upgraded. Multiple upgrade reactions
  // could be enqueued because the state changes in step 10 of upgrades.
  // https://html.spec.whatwg.org/multipage/scripting.html#upgrades
  if (element->GetCustomElementState() == CustomElementState::kUndefined)
    definition_->Upgrade(element);
}

}  // namespace blink
