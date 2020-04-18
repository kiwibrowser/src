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

#include "third_party/blink/renderer/core/html/custom/v0_custom_element_callback_invocation.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/html/custom/v0_custom_element_scheduler.h"

namespace blink {

class AttachedDetachedInvocation final
    : public V0CustomElementCallbackInvocation {
 public:
  AttachedDetachedInvocation(
      V0CustomElementLifecycleCallbacks*,
      V0CustomElementLifecycleCallbacks::CallbackType which);

 private:
  void Dispatch(Element*) override;

  V0CustomElementLifecycleCallbacks::CallbackType which_;
};

AttachedDetachedInvocation::AttachedDetachedInvocation(
    V0CustomElementLifecycleCallbacks* callbacks,
    V0CustomElementLifecycleCallbacks::CallbackType which)
    : V0CustomElementCallbackInvocation(callbacks), which_(which) {
  DCHECK(which_ == V0CustomElementLifecycleCallbacks::kAttachedCallback ||
         which_ == V0CustomElementLifecycleCallbacks::kDetachedCallback);
}

void AttachedDetachedInvocation::Dispatch(Element* element) {
  switch (which_) {
    case V0CustomElementLifecycleCallbacks::kAttachedCallback:
      Callbacks()->Attached(element);
      break;
    case V0CustomElementLifecycleCallbacks::kDetachedCallback:
      Callbacks()->Detached(element);
      break;
    default:
      NOTREACHED();
  }
}

class AttributeChangedInvocation final
    : public V0CustomElementCallbackInvocation {
 public:
  AttributeChangedInvocation(V0CustomElementLifecycleCallbacks*,
                             const AtomicString& name,
                             const AtomicString& old_value,
                             const AtomicString& new_value);

 private:
  void Dispatch(Element*) override;

  AtomicString name_;
  AtomicString old_value_;
  AtomicString new_value_;
};

AttributeChangedInvocation::AttributeChangedInvocation(
    V0CustomElementLifecycleCallbacks* callbacks,
    const AtomicString& name,
    const AtomicString& old_value,
    const AtomicString& new_value)
    : V0CustomElementCallbackInvocation(callbacks),
      name_(name),
      old_value_(old_value),
      new_value_(new_value) {}

void AttributeChangedInvocation::Dispatch(Element* element) {
  Callbacks()->AttributeChanged(element, name_, old_value_, new_value_);
}

class CreatedInvocation final : public V0CustomElementCallbackInvocation {
 public:
  explicit CreatedInvocation(V0CustomElementLifecycleCallbacks* callbacks)
      : V0CustomElementCallbackInvocation(callbacks) {}

 private:
  void Dispatch(Element*) override;
  bool IsCreatedCallback() const override { return true; }
};

void CreatedInvocation::Dispatch(Element* element) {
  if (element->isConnected() && element->GetDocument().domWindow())
    V0CustomElementScheduler::ScheduleCallback(
        Callbacks(), element,
        V0CustomElementLifecycleCallbacks::kAttachedCallback);
  Callbacks()->Created(element);
}

V0CustomElementCallbackInvocation*
V0CustomElementCallbackInvocation::CreateInvocation(
    V0CustomElementLifecycleCallbacks* callbacks,
    V0CustomElementLifecycleCallbacks::CallbackType which) {
  switch (which) {
    case V0CustomElementLifecycleCallbacks::kCreatedCallback:
      return new CreatedInvocation(callbacks);

    case V0CustomElementLifecycleCallbacks::kAttachedCallback:
    case V0CustomElementLifecycleCallbacks::kDetachedCallback:
      return new AttachedDetachedInvocation(callbacks, which);
    default:
      NOTREACHED();
      return nullptr;
  }
}

V0CustomElementCallbackInvocation*
V0CustomElementCallbackInvocation::CreateAttributeChangedInvocation(
    V0CustomElementLifecycleCallbacks* callbacks,
    const AtomicString& name,
    const AtomicString& old_value,
    const AtomicString& new_value) {
  return new AttributeChangedInvocation(callbacks, name, old_value, new_value);
}

void V0CustomElementCallbackInvocation::Trace(blink::Visitor* visitor) {
  visitor->Trace(callbacks_);
  V0CustomElementProcessingStep::Trace(visitor);
}

}  // namespace blink
