/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V0_CUSTOM_ELEMENT_CONSTRUCTOR_BUILDER_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V0_CUSTOM_ELEMENT_CONSTRUCTOR_BUILDER_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_v0_custom_element_lifecycle_callbacks.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/html/custom/v0_custom_element_lifecycle_callbacks.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "v8/include/v8.h"

namespace blink {

class V0CustomElementDefinition;
class Document;
class ElementRegistrationOptions;
class ExceptionState;
class QualifiedName;
struct WrapperTypeInfo;

// Handles the scripting-specific parts of the Custom Elements element
// registration algorithm and constructor generation algorithm. It is
// used in the implementation of those algorithms in
// Document::registerElement.
class V0CustomElementConstructorBuilder {
  WTF_MAKE_NONCOPYABLE(V0CustomElementConstructorBuilder);
  STACK_ALLOCATED();

 public:
  V0CustomElementConstructorBuilder(ScriptState*,
                                    const ElementRegistrationOptions&);

  // The builder accumulates state and may run script at specific
  // points. These methods must be called in order. When one fails
  // (returns false), the calls must stop.

  bool IsFeatureAllowed() const;
  bool ValidateOptions(const AtomicString& type,
                       QualifiedName& tag_name,
                       ExceptionState&);
  V0CustomElementLifecycleCallbacks* CreateCallbacks();
  bool CreateConstructor(Document*,
                         V0CustomElementDefinition*,
                         ExceptionState&);
  bool DidRegisterDefinition() const;

  // This method collects a return value for the bindings. It is
  // safe to call this method even if the builder failed; it will
  // return an empty value.
  ScriptValue BindingsReturnValue() const;

 private:
  bool HasValidPrototypeChainFor(const WrapperTypeInfo*) const;
  bool PrototypeIsValid(const AtomicString& type, ExceptionState&) const;
  v8::MaybeLocal<v8::Function> RetrieveCallback(const char* name);

  scoped_refptr<ScriptState> script_state_;
  const ElementRegistrationOptions& options_;
  v8::Local<v8::Object> prototype_;
  v8::Local<v8::Function> constructor_;
  Member<V8V0CustomElementLifecycleCallbacks> callbacks_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V0_CUSTOM_ELEMENT_CONSTRUCTOR_BUILDER_H_
