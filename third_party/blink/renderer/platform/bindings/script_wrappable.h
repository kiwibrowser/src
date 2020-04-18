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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_H_

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_base.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"
#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/type_traits.h"
#include "v8/include/v8.h"

namespace blink {

// ScriptWrappable provides a way to map from/to C++ DOM implementation to/from
// JavaScript object (platform object).  ToV8() converts a ScriptWrappable to
// a v8::Object and toScriptWrappable() converts a v8::Object back to
// a ScriptWrappable.  v8::Object as platform object is called "wrapper object".
// The wrapper object for the main world is stored in ScriptWrappable.  Wrapper
// objects for other worlds are stored in DOMWrapperMap.
class PLATFORM_EXPORT ScriptWrappable
    : public GarbageCollectedFinalized<ScriptWrappable>,
      public TraceWrapperBase {
  WTF_MAKE_NONCOPYABLE(ScriptWrappable);

 public:
  virtual ~ScriptWrappable() = default;

  virtual void Trace(blink::Visitor*);

  // Traces wrapper objects corresponding to this ScriptWrappable in all worlds.
  void TraceWrappers(ScriptWrappableVisitor*) const override;

  bool IsScriptWrappable() const override { return true; }

  const char* NameInHeapSnapshot() const override;

  template <typename T>
  T* ToImpl() {
    // All ScriptWrappables are managed by the Blink GC heap; check that
    // |T| is a garbage collected type.
    static_assert(
        sizeof(T) && WTF::IsGarbageCollectedType<T>::value,
        "Classes implementing ScriptWrappable must be garbage collected.");

// Check if T* is castable to ScriptWrappable*, which means T doesn't
// have two or more ScriptWrappable as superclasses. If T has two
// ScriptWrappable as superclasses, conversions from T* to
// ScriptWrappable* are ambiguous.
#if !defined(COMPILER_MSVC)
    // MSVC 2013 doesn't support static_assert + constexpr well.
    static_assert(!static_cast<ScriptWrappable*>(static_cast<T*>(nullptr)),
                  "Class T must not have two or more ScriptWrappable as its "
                  "superclasses.");
#endif
    return static_cast<T*>(this);
  }

  // Returns the WrapperTypeInfo of the instance.
  //
  // This method must be overridden by DEFINE_WRAPPERTYPEINFO macro.
  virtual const WrapperTypeInfo* GetWrapperTypeInfo() const = 0;

  // Creates and returns a new wrapper object.
  virtual v8::Local<v8::Object> Wrap(v8::Isolate*,
                                     v8::Local<v8::Object> creation_context);

  // Associates the instance with the given |wrapper| if this instance is not
  // yet associated with any wrapper.  Returns the wrapper already associated
  // or |wrapper| if not yet associated.
  // The caller should always use the returned value rather than |wrapper|.
  WARN_UNUSED_RESULT virtual v8::Local<v8::Object> AssociateWithWrapper(
      v8::Isolate*,
      const WrapperTypeInfo*,
      v8::Local<v8::Object> wrapper);

  // Returns true if the instance needs to be kept alive even when the
  // instance is unreachable from JavaScript.
  virtual bool HasPendingActivity() const { return false; }

  // Associates this instance with the given |wrapper| if this instance is not
  // yet associated with any wrapper.  Returns true if the given wrapper is
  // associated with this instance, or false if this instance is already
  // associated with a wrapper.  In the latter case, |wrapper| will be updated
  // to the existing wrapper.
  WARN_UNUSED_RESULT bool SetWrapper(v8::Isolate* isolate,
                                     const WrapperTypeInfo* wrapper_type_info,
                                     v8::Local<v8::Object>& wrapper) {
    DCHECK(!wrapper.IsEmpty());
    if (UNLIKELY(ContainsWrapper())) {
      wrapper = MainWorldWrapper(isolate);
      return false;
    }
    main_world_wrapper_.Set(isolate, wrapper);
    DCHECK(ContainsWrapper());
    wrapper_type_info->ConfigureWrapper(&main_world_wrapper_.Get());
    return true;
  }

  // Dissociates the wrapper, if any, from this instance.
  void UnsetWrapperIfAny() {
    if (ContainsWrapper()) {
      main_world_wrapper_.Get().Reset();
      WrapperTypeInfo::WrapperDestroyed();
    }
  }

  bool IsEqualTo(const v8::Local<v8::Object>& other) const {
    return main_world_wrapper_.Get() == other;
  }

  bool SetReturnValue(v8::ReturnValue<v8::Value> return_value) {
    return_value.Set(main_world_wrapper_.Get());
    return ContainsWrapper();
  }

  bool ContainsWrapper() const { return !main_world_wrapper_.IsEmpty(); }

 protected:
  ScriptWrappable() = default;

 private:
  // These classes are exceptionally allowed to use MainWorldWrapper().
  friend class DOMDataStore;
  friend class HeapSnaphotWrapperVisitor;
  friend class V8HiddenValue;
  friend class V8PrivateProperty;

  v8::Local<v8::Object> MainWorldWrapper(v8::Isolate* isolate) const {
    return main_world_wrapper_.NewLocal(isolate);
  }

  // Only use when really necessary, i.e., when passing over this
  // ScriptWrappable's reference to V8. Should only be needed by GC
  // infrastructure.
  const v8::Persistent<v8::Object>* RawMainWorldWrapper() const {
    return &main_world_wrapper_.Get();
  }

  TraceWrapperV8Reference<v8::Object> main_world_wrapper_;
};

// Defines |GetWrapperTypeInfo| virtual method which returns the WrapperTypeInfo
// of the instance. Also declares a static member of type WrapperTypeInfo, of
// which the definition is given by the IDL code generator.
//
// All the derived classes of ScriptWrappable, regardless of directly or
// indirectly, must write this macro in the class definition as long as the
// class has a corresponding .idl file.
#define DEFINE_WRAPPERTYPEINFO()                               \
 public:                                                       \
  const WrapperTypeInfo* GetWrapperTypeInfo() const override { \
    return &wrapper_type_info_;                                \
  }                                                            \
                                                               \
 private:                                                      \
  static const WrapperTypeInfo& wrapper_type_info_

// Declares |GetWrapperTypeInfo| method without definition.
//
// This macro is used for template classes. e.g. DOMTypedArray<>.
// To export such a template class X, we need to instantiate X with EXPORT_API,
// i.e. "extern template class EXPORT_API X;"
// However, once we instantiate X, we cannot specialize X after
// the instantiation. i.e. we will see "error: explicit specialization of ...
// after instantiation". So we cannot define X's s_wrapperTypeInfo in generated
// code by using specialization. Instead, we need to implement wrapperTypeInfo
// in X's cpp code, and instantiate X, i.e. "template class X;".
#define DECLARE_WRAPPERTYPEINFO()                             \
 public:                                                      \
  const WrapperTypeInfo* GetWrapperTypeInfo() const override; \
                                                              \
 private:                                                     \
  typedef void end_of_declare_wrappertypeinfo_t

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_H_
