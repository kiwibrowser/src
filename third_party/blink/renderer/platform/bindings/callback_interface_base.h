// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_CALLBACK_INTERFACE_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_CALLBACK_INTERFACE_BASE_H_

#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_base.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class V8PersistentCallbackInterfaceBase;

// CallbackInterfaceBase is the common base class of all the callback interface
// classes. Most importantly this class provides a way of type dispatching (e.g.
// overload resolutions, SFINAE technique, etc.) so that it's possible to
// distinguish callback interfaces from anything else. Also it provides a common
// implementation of callback interfaces.
//
// As the signatures of callback interface's operations vary, this class does
// not implement any operation. Subclasses will implement it.
class PLATFORM_EXPORT CallbackInterfaceBase
    : public GarbageCollectedFinalized<CallbackInterfaceBase>,
      public TraceWrapperBase {
 public:
  // Whether the callback interface is a "single operation callback interface"
  // or not.
  // https://heycam.github.io/webidl/#dfn-single-operation-callback-interface
  enum SingleOperationOrNot {
    kNotSingleOperation,
    kSingleOperation,
  };

  virtual ~CallbackInterfaceBase() = default;

  virtual void Trace(blink::Visitor*) {}
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "CallbackInterfaceBase";
  }

  v8::Isolate* GetIsolate() {
    return callback_relevant_script_state_->GetIsolate();
  }
  ScriptState* CallbackRelevantScriptState() {
    return callback_relevant_script_state_.get();
  }

  // NodeIteratorBase counts the invocation of those which are callable and
  // those which are not.
  bool IsCallbackObjectCallableForNodeIteratorBase() const {
    return IsCallbackObjectCallable();
  }

 protected:
  CallbackInterfaceBase(v8::Local<v8::Object> callback_object,
                        SingleOperationOrNot);

  v8::Local<v8::Object> CallbackObject() {
    return callback_object_.NewLocal(GetIsolate());
  }
  // Returns true iff the callback interface is a single operation callback
  // interface and the callback interface type value is callable.
  bool IsCallbackObjectCallable() const { return is_callback_object_callable_; }
  ScriptState* IncumbentScriptState() { return incumbent_script_state_.get(); }

 private:
  // The "callback interface type" value.
  TraceWrapperV8Reference<v8::Object> callback_object_;
  bool is_callback_object_callable_ = false;
  // The associated Realm of the callback interface type value. Note that the
  // callback interface type value can be different from the function object
  // to be invoked.
  scoped_refptr<ScriptState> callback_relevant_script_state_;
  // The callback context, i.e. the incumbent Realm when an ECMAScript value is
  // converted to an IDL value.
  // https://heycam.github.io/webidl/#dfn-callback-context
  scoped_refptr<ScriptState> incumbent_script_state_;

  friend class V8PersistentCallbackInterfaceBase;
  // ToV8 needs to call |CallbackObject| member function.
  friend v8::Local<v8::Value> ToV8(CallbackInterfaceBase* callback,
                                   v8::Local<v8::Object> creation_context,
                                   v8::Isolate*);
};

// V8PersistentCallbackInterfaceBase retains the underlying v8::Object of a
// CallbackInterfaceBase without wrapper-tracing. This class is necessary and
// useful where wrapper-tracing is not suitable. Remember that, as a nature of
// v8::Persistent, abuse of V8PersistentCallbackInterfaceBase would result in
// memory leak, so the use of V8PersistentCallbackInterfaceBase should be
// limited to those which are guaranteed to release the persistents in a finite
// time period.
class PLATFORM_EXPORT V8PersistentCallbackInterfaceBase
    : public GarbageCollectedFinalized<V8PersistentCallbackInterfaceBase> {
 public:
  virtual ~V8PersistentCallbackInterfaceBase() { v8_object_.Reset(); }

  virtual void Trace(blink::Visitor*);

  v8::Isolate* GetIsolate() { return callback_interface_->GetIsolate(); }

 protected:
  explicit V8PersistentCallbackInterfaceBase(CallbackInterfaceBase*);

  template <typename V8CallbackInterface>
  V8CallbackInterface* As() {
    static_assert(
        std::is_base_of<CallbackInterfaceBase, V8CallbackInterface>::value,
        "V8CallbackInterface must be a subclass of CallbackInterfaceBase.");
    return static_cast<V8CallbackInterface*>(callback_interface_.Get());
  }

 private:
  Member<CallbackInterfaceBase> callback_interface_;
  v8::Persistent<v8::Object> v8_object_;
};

// V8PersistentCallbackInterface<V8CallbackInterface> is a counter-part of
// V8CallbackInterface. While V8CallbackInterface uses wrapper-tracing,
// V8PersistentCallbackInterface<V8CallbackInterface> uses v8::Persistent to
// make the underlying v8::Object alive.
//
// Since the signatures of the operations vary depending on the IDL definition,
// the class definition is specialized and generated by the bindings code
// generator.
template <typename V8CallbackInterface>
class V8PersistentCallbackInterface;

// Converts the wrapper-tracing version of a callback interface to the
// v8::Persistent version of it.
template <typename V8CallbackInterface>
inline V8PersistentCallbackInterface<V8CallbackInterface>*
ToV8PersistentCallbackInterface(V8CallbackInterface* callback_interface) {
  static_assert(
      std::is_base_of<CallbackInterfaceBase, V8CallbackInterface>::value,
      "V8CallbackInterface must be a subclass of CallbackInterfaceBase.");
  return callback_interface
             ? new V8PersistentCallbackInterface<V8CallbackInterface>(
                   callback_interface)
             : nullptr;
}

// CallbackInterfaceBase is designed to be used with wrapper-tracing. As
// blink::Persistent does not perform wrapper-tracing, use of |WrapPersistent|
// for callback interfaces is likely (if not always) misuse. Thus, this code
// prohibits such a use case. The call sites should explicitly use
// WrapPersistent(V8PersistentCallbackInterface<T>*).
Persistent<CallbackInterfaceBase> WrapPersistent(CallbackInterfaceBase*) =
    delete;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_CALLBACK_INTERFACE_BASE_H_
