/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DOM_WRAPPER_MAP_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DOM_WRAPPER_MAP_H_

#include <utility>

#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"
#include "third_party/blink/renderer/platform/wtf/compiler.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "v8/include/v8-util.h"
#include "v8/include/v8.h"

namespace blink {

// Maps from C++ objects to their corresponding JavaScript wrappers. See also
// DOMDataStore.
template <class KeyType>
class DOMWrapperMap {
  USING_FAST_MALLOC(DOMWrapperMap);

 public:
  explicit DOMWrapperMap(v8::Isolate* isolate)
      : isolate_(isolate), map_(isolate) {}

  v8::Local<v8::Object> NewLocal(v8::Isolate* isolate, KeyType* key) {
    return map_.Get(key);
  }

  bool SetReturnValueFrom(v8::ReturnValue<v8::Value> return_value,
                          KeyType* key) {
    return map_.SetReturnValue(key, return_value);
  }

  void SetReference(v8::Isolate* isolate,
                    const v8::Persistent<v8::Object>& parent,
                    KeyType* key) {
    map_.SetReference(key, parent);
  }

  bool ContainsKey(const KeyType* key) {
    return map_.Contains(const_cast<KeyType*>(key));
  }

  WARN_UNUSED_RESULT bool Set(KeyType* key,
                              const WrapperTypeInfo* wrapper_type_info,
                              v8::Local<v8::Object>& wrapper) {
    if (UNLIKELY(ContainsKey(key))) {
      wrapper = NewLocal(isolate_, key);
      return false;
    }
    v8::Global<v8::Object> global(isolate_, wrapper);
    wrapper_type_info->ConfigureWrapper(&global);
    map_.Set(key, std::move(global));
    return true;
  }

  void RemoveIfAny(KeyType* key) {
    if (ContainsKey(key))
      map_.Remove(key);
  }

  void Clear() { map_.Clear(); }

  void RemoveAndDispose(KeyType* key) {
    DCHECK(ContainsKey(key));
    map_.Remove(key);
  }

  void MarkWrapper(KeyType* object) {
    map_.RegisterExternallyReferencedObject(object);
  }

 private:
  class PersistentValueMapTraits {
    STATIC_ONLY(PersistentValueMapTraits);

   public:
    // Map traits:
    //
    // DOMWrapperMap is NOT responsible to make |KeyType|s alive, so uses
    // UntracedMember<KeyType> as the key type of the internal storage.
    // |KeyType|s will be made alive by V8 wrapper objects.
    typedef HashMap<UntracedMember<KeyType>, v8::PersistentContainerValue> Impl;
    typedef typename Impl::iterator Iterator;
    static size_t Size(const Impl* impl) { return impl->size(); }
    static bool Empty(Impl* impl) { return impl->IsEmpty(); }
    static void Swap(Impl& impl, Impl& other) { impl.swap(other); }
    static Iterator Begin(Impl* impl) { return impl->begin(); }
    static Iterator End(Impl* impl) { return impl->end(); }
    static v8::PersistentContainerValue Value(Iterator& iter) {
      return iter->value;
    }
    static KeyType* Key(Iterator& iter) { return iter->key; }
    static v8::PersistentContainerValue
    Set(Impl* impl, KeyType* key, v8::PersistentContainerValue value) {
      v8::PersistentContainerValue old_value = Get(impl, key);
      impl->Set(key, value);
      return old_value;
    }
    static v8::PersistentContainerValue Get(const Impl* impl, KeyType* key) {
      return impl->at(key);
    }

    static v8::PersistentContainerValue Remove(Impl* impl, KeyType* key) {
      return impl->Take(key);
    }

    // Weak traits:
    static const v8::PersistentContainerCallbackType kCallbackType =
        v8::kWeakWithInternalFields;
    typedef v8::GlobalValueMap<KeyType*, v8::Object, PersistentValueMapTraits>
        MapType;
    typedef MapType WeakCallbackDataType;

    static WeakCallbackDataType* WeakCallbackParameter(
        MapType* map,
        KeyType* key,
        v8::Local<v8::Object>& value) {
      return map;
    }

    static void DisposeCallbackData(WeakCallbackDataType* callback_data) {}

    static MapType* MapFromWeakCallbackInfo(
        const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {
      return data.GetParameter();
    }

    static KeyType* KeyFromWeakCallbackInfo(
        const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {
      return reinterpret_cast<KeyType*>(
          data.GetInternalField(kV8DOMWrapperObjectIndex));
    }

    static void OnWeakCallback(
        const v8::WeakCallbackInfo<WeakCallbackDataType>&) {}

    static void Dispose(v8::Isolate*, v8::Global<v8::Object>, KeyType*);

    static void DisposeWeak(const v8::WeakCallbackInfo<WeakCallbackDataType>&);
  };

  v8::Isolate* isolate_;
  typename PersistentValueMapTraits::MapType map_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DOM_WRAPPER_MAP_H_
