/*
 * Copyright (C) 2006, 2007, 2008, 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_LAZY_EVENT_LISTENER_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_LAZY_EVENT_LISTENER_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_abstract_event_listener.h"
#include "third_party/blink/renderer/platform/wtf/text/text_position.h"
#include "v8/include/v8.h"

namespace blink {

class Event;
class Node;

// V8LazyEventListener is a wrapper for a JavaScript code string that is
// compiled and evaluated when an event is fired.  A V8LazyEventListener is
// either a HTML or SVG event handler.
class V8LazyEventListener final : public V8AbstractEventListener {
 public:
  static V8LazyEventListener* Create(const AtomicString& function_name,
                                     const AtomicString& event_parameter_name,
                                     const String& code,
                                     const String& source_url,
                                     const TextPosition& position,
                                     Node* node,
                                     v8::Isolate* isolate) {
    return new V8LazyEventListener(isolate, function_name, event_parameter_name,
                                   code, source_url, position, node);
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(node_);
    V8AbstractEventListener::Trace(visitor);
  }

  const String& Code() const override { return code_; }

 protected:
  v8::Local<v8::Object> GetListenerObjectInternal(ExecutionContext*) override;

 private:
  V8LazyEventListener(v8::Isolate*,
                      const AtomicString& function_name,
                      const AtomicString& event_parameter_name,
                      const String& code,
                      const String source_url,
                      const TextPosition&,
                      Node*);

  v8::Local<v8::Value> CallListenerFunction(ScriptState*,
                                            v8::Local<v8::Value>,
                                            Event*) override;

  // Return true, so that event handlers from markup are not cloned twice when
  // building the shadow tree for SVGUseElements.
  bool WasCreatedFromMarkup() const override { return true; }

  void CompileScript(ScriptState*, ExecutionContext*);

  void FireErrorEvent(v8::Local<v8::Context>,
                      ExecutionContext*,
                      v8::Local<v8::Message>);

  bool was_compilation_failed_;
  AtomicString function_name_;
  AtomicString event_parameter_name_;
  String code_;
  String source_url_;
  Member<Node> node_;
  TextPosition position_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_LAZY_EVENT_LISTENER_H_
