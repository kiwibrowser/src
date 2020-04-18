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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_VISITOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_VISITOR_H_

#include <memory>
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/hash_traits.h"
#include "third_party/blink/renderer/platform/wtf/type_traits.h"
#include "v8/include/v8.h"

namespace blink {

template <typename T>
class GarbageCollected;
template <typename T>
class DOMWrapperMap;
template <typename T>
class TraceTrait;
class ThreadState;
class Visitor;
template <typename T>
class SameThreadCheckedMember;
class ScriptWrappable;
template <typename T>
class TraceWrapperMember;
template <typename T>
class TraceWrapperV8Reference;

// The TraceMethodDelegate is used to convert a trace method for type T to a
// TraceCallback.  This allows us to pass a type's trace method as a parameter
// to the PersistentNode constructor. The PersistentNode constructor needs the
// specific trace method due an issue with the Windows compiler which
// instantiates even unused variables. This causes problems
// in header files where we have only forward declarations of classes.
template <typename T, void (T::*method)(Visitor*)>
struct TraceMethodDelegate {
  STATIC_ONLY(TraceMethodDelegate);
  static void Trampoline(Visitor* visitor, void* self) {
    (reinterpret_cast<T*>(self)->*method)(visitor);
  }
};

// Visitor is used to traverse Oilpan's object graph.
class PLATFORM_EXPORT Visitor {
 public:
  explicit Visitor(ThreadState* state) : state_(state) {}
  virtual ~Visitor() = default;

  inline ThreadState* State() const { return state_; }
  inline ThreadHeap& Heap() const { return state_->Heap(); }

  // Static visitor implementation forwarding to dynamic interface.

  // Member version of the one-argument templated trace method.
  template <typename T>
  void Trace(const Member<T>& t) {
    DCHECK(!t.IsHashTableDeletedValue());
    Trace(t.Get());
  }

  template <typename T>
  void Trace(const SameThreadCheckedMember<T>& t) {
    Trace(*(static_cast<const Member<T>*>(&t)));
  }

  // Fallback methods used only when we need to trace raw pointers of T. This is
  // the case when a member is a union where we do not support members.
  template <typename T>
  void Trace(const T* t) {
    Trace(const_cast<T*>(t));
  }

  template <typename T>
  void Trace(T* t) {
    static_assert(sizeof(T), "T must be fully defined");
    static_assert(IsGarbageCollectedType<T>::value,
                  "T needs to be a garbage collected object");
    if (!t)
      return;
    Visit(const_cast<void*>(reinterpret_cast<const void*>(t)),
          TraceDescriptorFor(t));
  }

  template <typename T>
  void TraceBackingStoreStrongly(T* backing_store, T** backing_store_slot) {
    static_assert(sizeof(T), "T must be fully defined");
    static_assert(IsGarbageCollectedType<T>::value,
                  "T needs to be a garbage collected object");

    if (!backing_store)
      return;
    VisitBackingStoreStrongly(reinterpret_cast<void*>(backing_store),
                              reinterpret_cast<void**>(backing_store_slot),
                              TraceDescriptorFor(backing_store));
  }

  template <typename T>
  void TraceBackingStoreWeakly(T* backing_store,
                               T** backing_store_slot,
                               WeakCallback callback,
                               void* parameter) {
    static_assert(sizeof(T), "T must be fully defined");
    static_assert(IsGarbageCollectedType<T>::value,
                  "T needs to be a garbage collected object");

    if (!backing_store)
      return;
    VisitBackingStoreWeakly(reinterpret_cast<void*>(backing_store),
                            reinterpret_cast<void**>(backing_store_slot),
                            TraceTrait<T>::GetTraceDescriptor(
                                reinterpret_cast<void*>(backing_store)),
                            callback, parameter);
  }

  template <typename T>
  void TraceBackingStoreOnly(T* backing_store, T** backing_store_slot) {
    static_assert(sizeof(T), "T must be fully defined");
    static_assert(IsGarbageCollectedType<T>::value,
                  "T needs to be a garbage collected object");

    if (!backing_store)
      return;
    VisitBackingStoreOnly(reinterpret_cast<void*>(backing_store),
                          reinterpret_cast<void**>(backing_store_slot));
  }

  // WeakMember version of the templated trace method. It doesn't keep
  // the traced thing alive, but will write null to the WeakMember later
  // if the pointed-to object is dead. It's lying for this to be const,
  // but the overloading resolver prioritizes constness too high when
  // picking the correct overload, so all these trace methods have to have
  // the same constness on their argument to allow the type to decide.
  template <typename T>
  void Trace(const WeakMember<T>& t) {
    static_assert(sizeof(T), "T must be fully defined");
    static_assert(IsGarbageCollectedType<T>::value,
                  "T needs to be a garbage collected object");

    if (!t.Get())
      return;

    DCHECK(!t.IsHashTableDeletedValue());
    VisitWeak(const_cast<void*>(reinterpret_cast<const void*>(t.Get())),
              reinterpret_cast<void**>(
                  const_cast<typename std::remove_const<T>::type**>(t.Cell())),
              TraceTrait<T>::GetTraceDescriptor(
                  const_cast<void*>(reinterpret_cast<const void*>(t.Get()))),
              &HandleWeakCell<T>);
  }

  // Fallback trace method for part objects to allow individual trace methods
  // to trace through a part object with visitor->trace(m_partObject). This
  // takes a const argument, because otherwise it will match too eagerly: a
  // non-const argument would match a non-const Vector<T>& argument better
  // than the specialization that takes const Vector<T>&. For a similar reason,
  // the other specializations take a const argument even though they are
  // usually used with non-const arguments, otherwise this function would match
  // too well.
  template <typename T>
  void Trace(const T& t) {
    static_assert(sizeof(T), "T must be fully defined");
    if (std::is_polymorphic<T>::value) {
      intptr_t vtable = *reinterpret_cast<const intptr_t*>(&t);
      if (!vtable)
        return;
    }
    TraceTrait<T>::Trace(this, &const_cast<T&>(t));
  }

  // Registers a callback for custom weakness.
  template <typename T, void (T::*method)(Visitor*)>
  void RegisterWeakMembers(const T* obj) {
    RegisterWeakCallback(const_cast<T*>(obj),
                         &TraceMethodDelegate<T, method>::Trampoline);
  }

  // Cross-component tracing interface.

  template <typename T>
  void Trace(const TraceWrapperMember<T>& t) {
    DCHECK(!t.IsHashTableDeletedValue());
    TraceWithWrappers(t.Get());
  }

  template <typename T>
  void TraceWithWrappers(T* t) {
    static_assert(sizeof(T), "T must be fully defined");
    static_assert(IsGarbageCollectedType<T>::value,
                  "T needs to be a garbage collected object");
    if (!t)
      return;

    // Dispatch two both, the TraceDescritpor and the TraceWrapperDescriptor,
    // versions of the visitor. This way the wrapper-tracing world can ignore
    // the TraceDescriptor versions.
    Visit(const_cast<void*>(reinterpret_cast<const void*>(t)),
          TraceDescriptorFor(t));
    Visit(const_cast<void*>(reinterpret_cast<const void*>(t)),
          TraceWrapperDescriptorFor(t));
  }

  void Trace(DOMWrapperMap<ScriptWrappable>* wrapper_map,
             const ScriptWrappable* key) {
    Visit(wrapper_map, key);
  }

  template <typename V8Type>
  void Trace(const TraceWrapperV8Reference<V8Type>& v8reference) {
    Visit(v8reference.template Cast<v8::Value>());
  }

  // Dynamic visitor interface.

  // Visits an object through a strong reference.
  virtual void Visit(void*, TraceDescriptor) = 0;
  // Subgraph of objects that are interested in wrappers. Note that the same
  // object is also passed to Visit(void*, TraceDescriptor).
  // TODO(mlippautz): Remove this visit method once wrapper tracing also uses
  // Trace() instead of TraceWrappers().
  virtual void Visit(void*, TraceWrapperDescriptor) = 0;

  // Visits an object through a weak reference.
  virtual void VisitWeak(void*, void**, TraceDescriptor, WeakCallback) = 0;

  // Visitors for collection backing stores.
  virtual void VisitBackingStoreStrongly(void*, void**, TraceDescriptor) = 0;
  virtual void VisitBackingStoreWeakly(void*,
                                       void**,
                                       TraceDescriptor,
                                       WeakCallback,
                                       void*) = 0;
  virtual void VisitBackingStoreOnly(void*, void**) = 0;

  // Visits cross-component references to V8.

  virtual void Visit(const TraceWrapperV8Reference<v8::Value>&) = 0;
  virtual void Visit(DOMWrapperMap<ScriptWrappable>*,
                     const ScriptWrappable* key) = 0;

  // Registers backing store pointers so that they can be moved and properly
  // updated.
  virtual void RegisterBackingStoreCallback(void* backing_store,
                                            MovingObjectCallback,
                                            void* callback_data) = 0;

  // Used to register ephemeron callbacks.
  virtual bool RegisterWeakTable(const void* closure,
                                 EphemeronCallback iteration_callback) {
    return false;
  }

  // |WeakCallback| will usually use |ObjectAliveTrait| to figure out liveness
  // of any children of |closure|. Upon return from the callback all references
  // to dead objects must have been purged. Any operation that extends the
  // object graph, including allocation or reviving objects, is prohibited.
  // Clearing out additional pointers is allowed. Note that removing elements
  // from heap collections such as HeapHashSet can cause an allocation if the
  // backing store requires resizing. These collections know how to deal with
  // WeakMember elements though.
  virtual void RegisterWeakCallback(void* closure, WeakCallback) = 0;

 protected:
  template <typename T>
  static inline TraceDescriptor TraceDescriptorFor(const T* traceable) {
    return TraceTrait<T>::GetTraceDescriptor(const_cast<T*>(traceable));
  }

  template <typename T>
  static inline TraceWrapperDescriptor TraceWrapperDescriptorFor(
      const T* traceable) {
    return TraceTrait<T>::GetTraceWrapperDescriptor(const_cast<T*>(traceable));
  }

 private:
  template <typename T>
  static void HandleWeakCell(Visitor* self, void*);

  ThreadState* const state_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_VISITOR_H_
