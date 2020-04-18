/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_CUSTOM_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_CUSTOM_EVENT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/custom_event_init.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"

namespace blink {

class CORE_EXPORT CustomEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~CustomEvent() override;

  static CustomEvent* Create() { return new CustomEvent; }

  static CustomEvent* Create(ScriptState* script_state,
                             const AtomicString& type,
                             const CustomEventInit& initializer) {
    return new CustomEvent(script_state, type, initializer);
  }

  void initCustomEvent(ScriptState*,
                       const AtomicString& type,
                       bool bubbles,
                       bool cancelable,
                       const ScriptValue& detail);

  const AtomicString& InterfaceName() const override;

  ScriptValue detail(ScriptState*) const;

  void Trace(blink::Visitor*) override;

  void TraceWrappers(ScriptWrappableVisitor*) const override;

 private:
  CustomEvent();
  CustomEvent(ScriptState*,
              const AtomicString& type,
              const CustomEventInit& initializer);

  scoped_refptr<DOMWrapperWorld> world_;
  TraceWrapperV8Reference<v8::Value> detail_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EVENTS_CUSTOM_EVENT_H_
