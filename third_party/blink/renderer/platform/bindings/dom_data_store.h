/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DOM_DATA_STORE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DOM_DATA_STORE_H_

#include <memory>

#include "base/optional.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_map.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable_marking_visitor.h"
#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/stack_util.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "v8/include/v8.h"

namespace blink {

// Holds a DOMWrapperMap specialized to map between ScriptWrappable objects and
// their wrappers and provides an API to perform common operations with this map
// and manage wrappers in a single world. Each world (DOMWrapperWorld) holds a
// single DOMWrapperMap instance to hold wrappers only for that world.
class DOMDataStore {
  WTF_MAKE_NONCOPYABLE(DOMDataStore);
  USING_FAST_MALLOC(DOMDataStore);

 public:
  DOMDataStore(v8::Isolate* isolate, bool is_main_world)
      : is_main_world_(is_main_world) {
    // We never use |m_wrapperMap| when it's the main world.
    if (!is_main_world)
      wrapper_map_.emplace(isolate);
  }

  static DOMDataStore& Current(v8::Isolate* isolate) {
    return DOMWrapperWorld::Current(isolate).DomDataStore();
  }

  static bool SetReturnValue(v8::ReturnValue<v8::Value> return_value,
                             ScriptWrappable* object) {
    if (CanUseMainWorldWrapper())
      return object->SetReturnValue(return_value);
    return Current(return_value.GetIsolate())
        .SetReturnValueFrom(return_value, object);
  }

  static bool SetReturnValueForMainWorld(
      v8::ReturnValue<v8::Value> return_value,
      ScriptWrappable* object) {
    return object->SetReturnValue(return_value);
  }

  static bool SetReturnValueFast(v8::ReturnValue<v8::Value> return_value,
                                 ScriptWrappable* object,
                                 v8::Local<v8::Object> holder,
                                 const ScriptWrappable* wrappable) {
    if (CanUseMainWorldWrapper()
        // The second fastest way to check if we're in the main world is to
        // check if the wrappable's wrapper is the same as the holder.
        || HolderContainsWrapper(holder, wrappable))
      return object->SetReturnValue(return_value);
    return Current(return_value.GetIsolate())
        .SetReturnValueFrom(return_value, object);
  }

  static v8::Local<v8::Object> GetWrapper(ScriptWrappable* object,
                                          v8::Isolate* isolate) {
    if (CanUseMainWorldWrapper())
      return object->MainWorldWrapper(isolate);
    return Current(isolate).Get(object, isolate);
  }

  // Associates the given |object| with the given |wrapper| if the object is
  // not yet associated with any wrapper.  Returns true if the given wrapper
  // is associated with the object, or false if the object is already
  // associated with a wrapper.  In the latter case, |wrapper| will be updated
  // to the existing wrapper.
  WARN_UNUSED_RESULT static bool SetWrapper(
      v8::Isolate* isolate,
      ScriptWrappable* object,
      const WrapperTypeInfo* wrapper_type_info,
      v8::Local<v8::Object>& wrapper) {
    if (CanUseMainWorldWrapper())
      return object->SetWrapper(isolate, wrapper_type_info, wrapper);
    return Current(isolate).Set(isolate, object, wrapper_type_info, wrapper);
  }

  static bool ContainsWrapper(const ScriptWrappable* object,
                              v8::Isolate* isolate) {
    return Current(isolate).ContainsWrapper(object);
  }

  v8::Local<v8::Object> Get(ScriptWrappable* object, v8::Isolate* isolate) {
    if (is_main_world_)
      return object->MainWorldWrapper(isolate);
    return wrapper_map_->NewLocal(isolate, object);
  }

  WARN_UNUSED_RESULT bool Set(v8::Isolate* isolate,
                              ScriptWrappable* object,
                              const WrapperTypeInfo* wrapper_type_info,
                              v8::Local<v8::Object>& wrapper) {
    DCHECK(object);
    DCHECK(!wrapper.IsEmpty());
    if (is_main_world_)
      return object->SetWrapper(isolate, wrapper_type_info, wrapper);
    bool updated = wrapper_map_->Set(object, wrapper_type_info, wrapper);
    if (updated) {
      ScriptWrappableMarkingVisitor::WriteBarrier(
          isolate, &wrapper_map_.value(), object);
    }
    return updated;
  }

  void Trace(const ScriptWrappable* script_wrappable, Visitor* visitor) {
    visitor->Trace(&wrapper_map_.value(), script_wrappable);
  }

  void TraceWrappers(const ScriptWrappable* script_wrappable,
                     ScriptWrappableVisitor* visitor) {
    visitor->TraceWrappers(&wrapper_map_.value(), script_wrappable);
  }

  void MarkWrapper(ScriptWrappable* script_wrappable) {
    wrapper_map_->MarkWrapper(script_wrappable);
  }

  // Dissociates a wrapper, if any, from |script_wrappable|.
  void UnsetWrapperIfAny(ScriptWrappable* script_wrappable) {
    DCHECK(!is_main_world_);
    wrapper_map_->RemoveIfAny(script_wrappable);
  }

  bool SetReturnValueFrom(v8::ReturnValue<v8::Value> return_value,
                          ScriptWrappable* object) {
    if (is_main_world_)
      return object->SetReturnValue(return_value);
    return wrapper_map_->SetReturnValueFrom(return_value, object);
  }

  bool ContainsWrapper(const ScriptWrappable* object) {
    if (is_main_world_)
      return object->ContainsWrapper();
    return wrapper_map_->ContainsKey(object);
  }

 private:
  // We can use a wrapper stored in a ScriptWrappable when we're in the main
  // world.  This method does the fast check if we're in the main world. If this
  // method returns true, it is guaranteed that we're in the main world. On the
  // other hand, if this method returns false, nothing is guaranteed (we might
  // be in the main world).
  static bool CanUseMainWorldWrapper() {
    return !WTF::MayNotBeMainThread() &&
           !DOMWrapperWorld::NonMainWorldsExistInMainThread();
  }

  static bool HolderContainsWrapper(v8::Local<v8::Object> holder,
                                    const ScriptWrappable* wrappable) {
    // Verify our assumptions about the main world.
    DCHECK(wrappable);
    DCHECK(!wrappable->ContainsWrapper() || !wrappable->IsEqualTo(holder) ||
           Current(v8::Isolate::GetCurrent()).is_main_world_);
    return wrappable->IsEqualTo(holder);
  }

  bool is_main_world_;
  base::Optional<DOMWrapperMap<ScriptWrappable>> wrapper_map_;
};

template <>
inline void DOMWrapperMap<ScriptWrappable>::PersistentValueMapTraits::Dispose(
    v8::Isolate*,
    v8::Global<v8::Object>,
    ScriptWrappable*) {
  WrapperTypeInfo::WrapperDestroyed();
}

template <>
inline void
DOMWrapperMap<ScriptWrappable>::PersistentValueMapTraits::DisposeWeak(
    const v8::WeakCallbackInfo<WeakCallbackDataType>&) {
  WrapperTypeInfo::WrapperDestroyed();
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DOM_DATA_STORE_H_
