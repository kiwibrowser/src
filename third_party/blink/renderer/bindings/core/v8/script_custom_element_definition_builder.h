// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_CUSTOM_ELEMENT_DEFINITION_BUILDER_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_CUSTOM_ELEMENT_DEFINITION_BUILDER_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/html/custom/custom_element_definition_builder.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"
#include "v8/include/v8.h"

namespace blink {

class CSSStyleSheet;
class CustomElementRegistry;
class ExceptionState;
class ScriptState;
class ScriptValue;

class CORE_EXPORT ScriptCustomElementDefinitionBuilder
    : public CustomElementDefinitionBuilder {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(ScriptCustomElementDefinitionBuilder);

 public:
  ScriptCustomElementDefinitionBuilder(
      ScriptState*,
      CustomElementRegistry*,
      CSSStyleSheet*,
      const ScriptValue& constructor_script_value,
      ExceptionState&);
  ~ScriptCustomElementDefinitionBuilder() = default;

  bool CheckConstructorIntrinsics() override;
  bool CheckConstructorNotRegistered() override;
  bool CheckPrototype() override;
  bool RememberOriginalProperties() override;
  CustomElementDefinition* Build(const CustomElementDescriptor&,
                                 CustomElementDefinition::Id) override;

 private:
  static ScriptCustomElementDefinitionBuilder* stack_;

  scoped_refptr<ScriptState> script_state_;
  Member<CustomElementRegistry> registry_;
  const Member<CSSStyleSheet> default_style_sheet_;
  v8::Local<v8::Value> constructor_value_;
  v8::Local<v8::Object> constructor_;
  v8::Local<v8::Object> prototype_;
  v8::Local<v8::Function> connected_callback_;
  v8::Local<v8::Function> disconnected_callback_;
  v8::Local<v8::Function> adopted_callback_;
  v8::Local<v8::Function> attribute_changed_callback_;
  HashSet<AtomicString> observed_attributes_;
  ExceptionState& exception_state_;

  bool ValueForName(v8::Isolate*,
                    v8::Local<v8::Context>&,
                    const v8::TryCatch&,
                    const v8::Local<v8::Object>&,
                    const StringView&,
                    v8::Local<v8::Value>&) const;
  bool CallableForName(v8::Isolate*,
                       v8::Local<v8::Context>&,
                       const v8::TryCatch&,
                       const StringView&,
                       v8::Local<v8::Function>&) const;
  bool RetrieveObservedAttributes(v8::Isolate*,
                                  v8::Local<v8::Context>&,
                                  const v8::TryCatch&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_CUSTOM_ELEMENT_DEFINITION_BUILDER_H_
