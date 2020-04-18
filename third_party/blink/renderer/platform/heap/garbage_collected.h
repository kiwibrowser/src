// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_GARBAGE_COLLECTED_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_GARBAGE_COLLECTED_H_

#include "base/macros.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/type_traits.h"

namespace blink {

template <typename T>
class GarbageCollected;
class HeapObjectHeader;

// GC_PLUGIN_IGNORE is used to make the plugin ignore a particular class or
// field when checking for proper usage.  When using GC_PLUGIN_IGNORE
// a bug-number should be provided as an argument where the bug describes
// what needs to happen to remove the GC_PLUGIN_IGNORE again.
#if defined(__clang__)
#define GC_PLUGIN_IGNORE(bug) \
  __attribute__((annotate("blink_gc_plugin_ignore")))
#else
#define GC_PLUGIN_IGNORE(bug)
#endif

// Template to determine if a class is a GarbageCollectedMixin by checking if it
// has IsGarbageCollectedMixinMarker
template <typename T>
struct IsGarbageCollectedMixin {
 private:
  typedef char YesType;
  struct NoType {
    char padding[8];
  };

  template <typename U>
  static YesType CheckMarker(typename U::IsGarbageCollectedMixinMarker*);
  template <typename U>
  static NoType CheckMarker(...);

 public:
  static const bool value = sizeof(CheckMarker<T>(nullptr)) == sizeof(YesType);
};

struct TraceDescriptor {
  STACK_ALLOCATED();
  void* base_object_payload;
  TraceCallback callback;
  bool can_trace_eagerly;
};

struct TraceWrapperDescriptor {
  STACK_ALLOCATED();
  void* base_object_payload;
  TraceWrappersCallback trace_wrappers_callback;
};

// The GarbageCollectedMixin interface and helper macro
// USING_GARBAGE_COLLECTED_MIXIN can be used to automatically define
// TraceTrait/ObjectAliveTrait on non-leftmost deriving classes
// which need to be garbage collected.
//
// Consider the following case:
// class B {};
// class A : public GarbageCollected, public B {};
//
// We can't correctly handle "Member<B> p = &a" as we can't compute addr of
// object header statically. This can be solved by using GarbageCollectedMixin:
// class B : public GarbageCollectedMixin {};
// class A : public GarbageCollected, public B {
//   USING_GARBAGE_COLLECTED_MIXIN(A);
// };
//
// With the helper, as long as we are using Member<B>, TypeTrait<B> will
// dispatch dynamically to retrieve the necessary tracing and header methods.
// Note that this is only enabled for Member<B>. For Member<A> which we can
// compute the necessary methods and pointers statically and this dynamic
// dispatch is not used.
class PLATFORM_EXPORT GarbageCollectedMixin {
 public:
  typedef int IsGarbageCollectedMixinMarker;
  virtual void Trace(Visitor*) {}
  // Provide default implementations that indicate that the vtable is not yet
  // set up properly. This way it is possible to get infos about mixins so that
  // these objects can processed later on. This is necessary as
  // not-fully-constructed mixin objects potentially require being processed
  // as part emitting a write barrier for incremental marking. See
  // IncrementalMarkingTest::WriteBarrierDuringMixinConstruction as an example.
  //
  // The not-fully-constructed objects are handled as follows:
  //   1. Write barrier or marking of not fully constructed mixin gets called.
  //   2. Default implementation of GetTraceDescriptor (and friends) returns
  //      kNotFullyConstructedObject as object base payload.
  //   3. Visitor (e.g. MarkingVisitor) can intercept that value and delay
  //      processing that object until the atomic pause.
  //   4. In the atomic phase, mark all not-fully-constructed objects that have
  //      found in the step 1.-3. conservatively.
  //
  // In general, delaying is required as write barriers are omitted in certain
  // scenarios, e.g., initializing stores. As a result, we cannot depend on the
  // write barriers for catching writes to member fields and thus have to
  // process the object (instead of just marking only the header).
  virtual HeapObjectHeader* GetHeapObjectHeader() const {
    return reinterpret_cast<HeapObjectHeader*>(
        BlinkGC::kNotFullyConstructedObject);
  }
  virtual TraceDescriptor GetTraceDescriptor() const {
    return {BlinkGC::kNotFullyConstructedObject, nullptr, false};
  }
  virtual TraceWrapperDescriptor GetTraceWrapperDescriptor() const {
    return {BlinkGC::kNotFullyConstructedObject, nullptr};
  }
};

#define DEFINE_GARBAGE_COLLECTED_MIXIN_METHODS(TYPE)                         \
 public:                                                                     \
  HeapObjectHeader* GetHeapObjectHeader() const override {                   \
    static_assert(                                                           \
        WTF::IsSubclassOfTemplate<typename std::remove_const<TYPE>::type,    \
                                  blink::GarbageCollected>::value,           \
        "only garbage collected objects can have garbage collected mixins"); \
    return HeapObjectHeader::FromPayload(static_cast<const TYPE*>(this));    \
  }                                                                          \
                                                                             \
  TraceDescriptor GetTraceDescriptor() const override {                      \
    return {const_cast<TYPE*>(static_cast<const TYPE*>(this)),               \
            TraceTrait<TYPE>::Trace, TraceEagerlyTrait<TYPE>::value};        \
  }                                                                          \
                                                                             \
  TraceWrapperDescriptor GetTraceWrapperDescriptor() const override {        \
    return {const_cast<TYPE*>(static_cast<const TYPE*>(this)),               \
            TraceTrait<TYPE>::TraceWrappers};                                \
  }                                                                          \
                                                                             \
 private:

// A C++ object's vptr will be initialized to its leftmost base's vtable after
// the constructors of all its subclasses have run, so if a subclass constructor
// tries to access any of the vtbl entries of its leftmost base prematurely,
// it'll find an as-yet incorrect vptr and fail. Which is exactly what a
// garbage collector will try to do if it tries to access the leftmost base
// while one of the subclass constructors of a GC mixin object triggers a GC.
// It is consequently not safe to allow any GCs while these objects are under
// (sub constructor) construction.
//
// To prevent GCs in that restricted window of a mixin object's construction:
//
//  - The initial allocation of the mixin object will enter a no GC scope.
//    This is done by overriding 'operator new' for mixin instances.
//  - When the constructor for the mixin is invoked, after all the
//    derived constructors have run, it will invoke the constructor
//    for a field whose only purpose is to leave the GC scope.
//    GarbageCollectedMixinConstructorMarker's constructor takes care of
//    this and the field is declared by way of USING_GARBAGE_COLLECTED_MIXIN().

#define DEFINE_GARBAGE_COLLECTED_MIXIN_CONSTRUCTOR_MARKER(TYPE)           \
 public:                                                                  \
  GC_PLUGIN_IGNORE("crbug.com/456823")                                    \
  NO_SANITIZE_UNRELATED_CAST void* operator new(size_t size) {            \
    CHECK_GE(kLargeObjectSizeThreshold, size)                             \
        << "GarbageCollectedMixin may not be a large object";             \
    void* object =                                                        \
        TYPE::AllocateObject(size, IsEagerlyFinalizedType<TYPE>::value);  \
    ThreadState* state =                                                  \
        ThreadStateFor<ThreadingTrait<TYPE>::kAffinity>::GetState();      \
    state->EnterGCForbiddenScopeIfNeeded(                                 \
        &(reinterpret_cast<TYPE*>(object)->mixin_constructor_marker_));   \
    return object;                                                        \
  }                                                                       \
  GarbageCollectedMixinConstructorMarker<ThreadingTrait<TYPE>::kAffinity> \
      mixin_constructor_marker_;                                          \
                                                                          \
 private:

// Mixins that wrap/nest others requires extra handling:
//
//  class A : public GarbageCollected<A>, public GarbageCollectedMixin {
//  USING_GARBAGE_COLLECTED_MIXIN(A);
//  ...
//  }'
//  public B final : public A, public SomeOtherMixinInterface {
//  USING_GARBAGE_COLLECTED_MIXIN(B);
//  ...
//  };
//
// The "operator new" for B will enter the forbidden GC scope, but
// upon construction, two GarbageCollectedMixinConstructorMarker constructors
// will run -- one for A (first) and another for B (secondly). Only
// the second one should leave the forbidden GC scope. This is realized by
// recording the address of B's GarbageCollectedMixinConstructorMarker
// when the "operator new" for B runs, and leaving the forbidden GC scope
// when the constructor of the recorded GarbageCollectedMixinConstructorMarker
// runs.
#define USING_GARBAGE_COLLECTED_MIXIN(TYPE)    \
  IS_GARBAGE_COLLECTED_TYPE();                 \
  DEFINE_GARBAGE_COLLECTED_MIXIN_METHODS(TYPE) \
  DEFINE_GARBAGE_COLLECTED_MIXIN_CONSTRUCTOR_MARKER(TYPE)

// An empty class with a constructor that's arranged invoked when all derived
// constructors of a mixin instance have completed and it is safe to allow GCs
// again. See AllocateObjectTrait<> comment for more.
//
// USING_GARBAGE_COLLECTED_MIXIN() declares a
// GarbageCollectedMixinConstructorMarker<> private field. By following Blink
// convention of using the macro at the top of a class declaration, its
// constructor will run first.
class GarbageCollectedMixinConstructorMarkerBase {};
template <ThreadAffinity affinity>
class GarbageCollectedMixinConstructorMarker
    : public GarbageCollectedMixinConstructorMarkerBase {
 public:
  GarbageCollectedMixinConstructorMarker() {
    // FIXME: if prompt conservative GCs are needed, forced GCs that
    // were denied while within this scope, could now be performed.
    // For now, assume the next out-of-line allocation request will
    // happen soon enough and take care of it. Mixin objects aren't
    // overly common.
    ThreadState* state = ThreadStateFor<affinity>::GetState();
    state->LeaveGCForbiddenScopeIfNeeded(this);
  }
};

// Merge two or more Mixins into one:
//
//  class A : public GarbageCollectedMixin {};
//  class B : public GarbageCollectedMixin {};
//  class C : public A, public B {
//    // C::GetTraceDescriptor is now ambiguous because there are two
//    candidates:
//    // A::GetTraceDescriptor and B::GetTraceDescriptor.  Ditto for other
//    functions.
//
//    MERGE_GARBAGE_COLLECTED_MIXINS();
//    // The macro defines C::GetTraceDescriptor, etc. so that they are no
//    longer
//    // ambiguous. USING_GARBAGE_COLLECTED_MIXIN(TYPE) overrides them later
//    // and provides the implementations.
//  };
#define MERGE_GARBAGE_COLLECTED_MIXINS()                                 \
 public:                                                                 \
  HeapObjectHeader* GetHeapObjectHeader() const override = 0;            \
  TraceDescriptor GetTraceDescriptor() const override = 0;               \
  TraceWrapperDescriptor GetTraceWrapperDescriptor() const override = 0; \
                                                                         \
 private:                                                                \
  using merge_garbage_collected_mixins_requires_semicolon = void

// Base class for objects allocated in the Blink garbage-collected heap.
//
// Defines a 'new' operator that allocates the memory in the heap.  'delete'
// should not be called on objects that inherit from GarbageCollected.
//
// Instances of GarbageCollected will *NOT* get finalized.  Their destructor
// will not be called.  Therefore, only classes that have trivial destructors
// with no semantic meaning (including all their subclasses) should inherit from
// GarbageCollected.  If there are non-trival destructors in a given class or
// any of its subclasses, GarbageCollectedFinalized should be used which
// guarantees that the destructor is called on an instance when the garbage
// collector determines that it is no longer reachable.
template <typename T>
class GarbageCollected;

// Base class for objects allocated in the Blink garbage-collected heap.
//
// Defines a 'new' operator that allocates the memory in the heap.  'delete'
// should not be called on objects that inherit from GarbageCollected.
//
// Instances of GarbageCollectedFinalized will have their destructor called when
// the garbage collector determines that the object is no longer reachable.
template <typename T>
class GarbageCollectedFinalized : public GarbageCollected<T> {
 protected:
  // finalizeGarbageCollectedObject is called when the object is freed from
  // the heap.  By default finalization means calling the destructor on the
  // object.  finalizeGarbageCollectedObject can be overridden to support
  // calling the destructor of a subclass.  This is useful for objects without
  // vtables that require explicit dispatching.  The name is intentionally a
  // bit long to make name conflicts less likely.
  void FinalizeGarbageCollectedObject() { static_cast<T*>(this)->~T(); }

  GarbageCollectedFinalized() = default;
  ~GarbageCollectedFinalized() = default;

  template <typename U>
  friend struct HasFinalizer;
  template <typename U, bool>
  friend struct FinalizerTraitImpl;

  DISALLOW_COPY_AND_ASSIGN(GarbageCollectedFinalized);
};

template <typename T,
          bool = WTF::IsSubclassOfTemplate<typename std::remove_const<T>::type,
                                           GarbageCollected>::value>
class NeedsAdjustPointer;

template <typename T>
class NeedsAdjustPointer<T, true> {
  static_assert(sizeof(T), "T must be fully defined");

 public:
  static const bool value = false;
};

template <typename T>
class NeedsAdjustPointer<T, false> {
  static_assert(sizeof(T), "T must be fully defined");

 public:
  static const bool value =
      IsGarbageCollectedMixin<typename std::remove_const<T>::type>::value;
};

// TODO(sof): migrate to wtf/TypeTraits.h
template <typename T>
class IsFullyDefined {
  using TrueType = char;
  struct FalseType {
    char dummy[2];
  };

  template <typename U, size_t sz = sizeof(U)>
  static TrueType IsSizeofKnown(U*);
  static FalseType IsSizeofKnown(...);
  static T& t_;

 public:
  static const bool value = sizeof(TrueType) == sizeof(IsSizeofKnown(&t_));
};

}  // namespace blink

#endif
