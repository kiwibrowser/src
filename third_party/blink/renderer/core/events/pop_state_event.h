/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
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
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_POP_STATE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_POP_STATE_EVENT_H_

#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/events/pop_state_event_init.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class History;
class SerializedScriptValue;

class PopStateEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~PopStateEvent() override;
  static PopStateEvent* Create();
  static PopStateEvent* Create(scoped_refptr<SerializedScriptValue>, History*);
  static PopStateEvent* Create(ScriptState*,
                               const AtomicString&,
                               const PopStateEventInit&);

  ScriptValue state(ScriptState*) const;
  SerializedScriptValue* SerializedState() const {
    return serialized_state_.get();
  }
  void SetSerializedState(scoped_refptr<SerializedScriptValue> state) {
    DCHECK(!serialized_state_);
    serialized_state_ = std::move(state);
  }
  History* GetHistory() const { return history_.Get(); }

  const AtomicString& InterfaceName() const override;

  void Trace(blink::Visitor*) override;

  void TraceWrappers(ScriptWrappableVisitor*) const override;

 private:
  PopStateEvent();
  PopStateEvent(ScriptState*, const AtomicString&, const PopStateEventInit&);
  PopStateEvent(scoped_refptr<SerializedScriptValue>, History*);

  scoped_refptr<SerializedScriptValue> serialized_state_;
  scoped_refptr<DOMWrapperWorld> world_;
  TraceWrapperV8Reference<v8::Value> state_;
  Member<History> history_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_POP_STATE_EVENT_H_
