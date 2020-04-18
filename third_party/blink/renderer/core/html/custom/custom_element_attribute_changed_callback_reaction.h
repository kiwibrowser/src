// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_ATTRIBUTE_CHANGED_CALLBACK_REACTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_ATTRIBUTE_CHANGED_CALLBACK_REACTION_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_reaction.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class CORE_EXPORT CustomElementAttributeChangedCallbackReaction final
    : public CustomElementReaction {
 public:
  CustomElementAttributeChangedCallbackReaction(CustomElementDefinition*,
                                                const QualifiedName&,
                                                const AtomicString& old_value,
                                                const AtomicString& new_value);

 private:
  void Invoke(Element*) override;

  QualifiedName name_;
  AtomicString old_value_;
  AtomicString new_value_;

  DISALLOW_COPY_AND_ASSIGN(CustomElementAttributeChangedCallbackReaction);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_ATTRIBUTE_CHANGED_CALLBACK_REACTION_H_
