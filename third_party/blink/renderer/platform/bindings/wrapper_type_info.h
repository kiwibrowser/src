/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_WRAPPER_TYPE_INFO_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_WRAPPER_TYPE_INFO_H_

#include "gin/public/wrapper_info.h"
#include "third_party/blink/renderer/platform/bindings/active_script_wrappable_base.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "v8/include/v8.h"

namespace blink {

class DOMWrapperWorld;
class ScriptWrappable;

ScriptWrappable* ToScriptWrappable(
    const v8::PersistentBase<v8::Object>& wrapper);
ScriptWrappable* ToScriptWrappable(v8::Local<v8::Object> wrapper);

static const int kV8DOMWrapperTypeIndex =
    static_cast<int>(gin::kWrapperInfoIndex);
static const int kV8DOMWrapperObjectIndex =
    static_cast<int>(gin::kEncodedValueIndex);
static const int kV8DefaultWrapperInternalFieldCount =
    static_cast<int>(gin::kNumberOfInternalFields);
static const int kV8PrototypeTypeIndex = 0;
static const int kV8PrototypeInternalFieldcount = 1;

typedef v8::Local<v8::FunctionTemplate> (
    *DomTemplateFunction)(v8::Isolate*, const DOMWrapperWorld&);
typedef ActiveScriptWrappableBase* (*ToActiveScriptWrappableFunction)(
    v8::Local<v8::Object>);
typedef void (*ResolveWrapperReachabilityFunction)(
    v8::Isolate*,
    ScriptWrappable*,
    const v8::Persistent<v8::Object>&);
typedef void (*InstallConditionalFeaturesFunction)(
    v8::Local<v8::Context>,
    const DOMWrapperWorld&,
    v8::Local<v8::Object>,
    v8::Local<v8::Object>,
    v8::Local<v8::Function>,
    v8::Local<v8::FunctionTemplate>);

// This struct provides a way to store a bunch of information that is helpful
// when unwrapping v8 objects. Each v8 bindings class has exactly one static
// WrapperTypeInfo member, so comparing pointers is a safe way to determine if
// types match.
struct WrapperTypeInfo {
  DISALLOW_NEW();

  enum WrapperTypePrototype {
    kWrapperTypeObjectPrototype,
    kWrapperTypeNoPrototype,  // For legacy callback interface
  };

  enum WrapperClassId {
    kNodeClassId = 1,  // NodeClassId must be smaller than ObjectClassId.
    kObjectClassId,
  };

  enum ActiveScriptWrappableInheritance {
    kNotInheritFromActiveScriptWrappable,
    kInheritFromActiveScriptWrappable,
  };

  static const WrapperTypeInfo* Unwrap(v8::Local<v8::Value> type_info_wrapper) {
    return reinterpret_cast<const WrapperTypeInfo*>(
        v8::External::Cast(*type_info_wrapper)->Value());
  }

  static void WrapperCreated() {
    ThreadState::Current()->Heap().HeapStats().IncreaseWrapperCount(1);
  }

  static void WrapperDestroyed() {
    ThreadHeapStats& heap_stats = ThreadState::Current()->Heap().HeapStats();
    heap_stats.DecreaseWrapperCount(1);
    heap_stats.IncreaseCollectedWrapperCount(1);
  }

  bool Equals(const WrapperTypeInfo* that) const { return this == that; }

  bool IsSubclass(const WrapperTypeInfo* that) const {
    for (const WrapperTypeInfo* current = this; current;
         current = current->parent_class) {
      if (current == that)
        return true;
    }

    return false;
  }

  void ConfigureWrapper(v8::PersistentBase<v8::Object>* wrapper) const {
    wrapper->SetWrapperClassId(wrapper_class_id);
  }

  v8::Local<v8::FunctionTemplate> domTemplate(
      v8::Isolate* isolate,
      const DOMWrapperWorld& world) const {
    return dom_template_function(isolate, world);
  }

  void InstallConditionalFeatures(
      v8::Local<v8::Context> context,
      const DOMWrapperWorld& world,
      v8::Local<v8::Object> instance_object,
      v8::Local<v8::Object> prototype_object,
      v8::Local<v8::Function> interface_object,
      v8::Local<v8::FunctionTemplate> interface_template) const {
    if (install_conditional_features_function) {
      install_conditional_features_function(context, world, instance_object,
                                            prototype_object, interface_object,
                                            interface_template);
    }
  }

  bool IsActiveScriptWrappable() const {
    return active_script_wrappable_inheritance ==
           kInheritFromActiveScriptWrappable;
  }

  // This field must be the first member of the struct WrapperTypeInfo.
  // See also static_assert() in .cpp file.
  const gin::GinEmbedder gin_embedder;

  DomTemplateFunction dom_template_function;
  InstallConditionalFeaturesFunction install_conditional_features_function;
  const char* const interface_name;
  const WrapperTypeInfo* parent_class;
  const unsigned wrapper_type_prototype : 2;  // WrapperTypePrototype
  const unsigned wrapper_class_id : 2;        // WrapperClassId
  const unsigned  // ActiveScriptWrappableInheritance
      active_script_wrappable_inheritance : 1;
};

template <typename T, int offset>
inline T* GetInternalField(const v8::PersistentBase<v8::Object>& persistent) {
  DCHECK_LT(offset, v8::Object::InternalFieldCount(persistent));
  return reinterpret_cast<T*>(
      v8::Object::GetAlignedPointerFromInternalField(persistent, offset));
}

template <typename T, int offset>
inline T* GetInternalField(v8::Local<v8::Object> wrapper) {
  DCHECK_LT(offset, wrapper->InternalFieldCount());
  return reinterpret_cast<T*>(
      wrapper->GetAlignedPointerFromInternalField(offset));
}

// The return value can be null if |wrapper| is a global proxy, which points to
// nothing while a navigation.
inline ScriptWrappable* ToScriptWrappable(
    const v8::PersistentBase<v8::Object>& wrapper) {
  return GetInternalField<ScriptWrappable, kV8DOMWrapperObjectIndex>(wrapper);
}

inline ScriptWrappable* ToScriptWrappable(v8::Local<v8::Object> wrapper) {
  return GetInternalField<ScriptWrappable, kV8DOMWrapperObjectIndex>(wrapper);
}

inline const WrapperTypeInfo* ToWrapperTypeInfo(
    const v8::PersistentBase<v8::Object>& wrapper) {
  return GetInternalField<WrapperTypeInfo, kV8DOMWrapperTypeIndex>(wrapper);
}

inline const WrapperTypeInfo* ToWrapperTypeInfo(v8::Local<v8::Object> wrapper) {
  return GetInternalField<WrapperTypeInfo, kV8DOMWrapperTypeIndex>(wrapper);
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_WRAPPER_TYPE_INFO_H_
