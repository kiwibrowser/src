// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_DISCONNECTED_CALLBACK_REACTION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_DISCONNECTED_CALLBACK_REACTION_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_reaction.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class CORE_EXPORT CustomElementDisconnectedCallbackReaction final
    : public CustomElementReaction {
 public:
  CustomElementDisconnectedCallbackReaction(CustomElementDefinition*);

 private:
  void Invoke(Element*) override;

  DISALLOW_COPY_AND_ASSIGN(CustomElementDisconnectedCallbackReaction);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_CUSTOM_ELEMENT_DISCONNECTED_CALLBACK_REACTION_H_
