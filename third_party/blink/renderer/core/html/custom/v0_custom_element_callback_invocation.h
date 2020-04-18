/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_V0_CUSTOM_ELEMENT_CALLBACK_INVOCATION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_V0_CUSTOM_ELEMENT_CALLBACK_INVOCATION_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/html/custom/v0_custom_element_lifecycle_callbacks.h"
#include "third_party/blink/renderer/core/html/custom/v0_custom_element_processing_step.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class V0CustomElementCallbackInvocation : public V0CustomElementProcessingStep {
 public:
  static V0CustomElementCallbackInvocation* CreateInvocation(
      V0CustomElementLifecycleCallbacks*,
      V0CustomElementLifecycleCallbacks::CallbackType);
  static V0CustomElementCallbackInvocation* CreateAttributeChangedInvocation(
      V0CustomElementLifecycleCallbacks*,
      const AtomicString& name,
      const AtomicString& old_value,
      const AtomicString& new_value);

 protected:
  V0CustomElementCallbackInvocation(
      V0CustomElementLifecycleCallbacks* callbacks)
      : callbacks_(callbacks) {}

  V0CustomElementLifecycleCallbacks* Callbacks() { return callbacks_.Get(); }

  void Trace(blink::Visitor*) override;

 private:
  Member<V0CustomElementLifecycleCallbacks> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(V0CustomElementCallbackInvocation);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_CUSTOM_V0_CUSTOM_ELEMENT_CALLBACK_INVOCATION_H_
