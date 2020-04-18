// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_FINALIZER_TRAITS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_FINALIZER_TRAITS_H_

#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace WTF {

template <typename ValueArg, typename Allocator>
class ListHashSetNode;

}  // namespace WTF

namespace blink {

// The FinalizerTraitImpl specifies how to finalize objects. Objects that
// inherit from GarbageCollectedFinalized are finalized by calling their
// |Finalize| method which by default will call the destructor on the object.
template <typename T, bool isGarbageCollectedFinalized>
struct FinalizerTraitImpl;

template <typename T>
struct FinalizerTraitImpl<T, true> {
  STATIC_ONLY(FinalizerTraitImpl);
  static void Finalize(void* obj) {
    static_assert(sizeof(T), "T must be fully defined");
    static_cast<T*>(obj)->FinalizeGarbageCollectedObject();
  };
};

template <typename T>
struct FinalizerTraitImpl<T, false> {
  STATIC_ONLY(FinalizerTraitImpl);
  static void Finalize(void* obj) {
    static_assert(sizeof(T), "T must be fully defined");
  };
};

// The FinalizerTrait is used to determine if a type requires finalization and
// what finalization means.
//
// By default classes that inherit from GarbageCollectedFinalized need
// finalization and finalization means calling the |Finalize| method of the
// object. The FinalizerTrait can be specialized if the default behavior is not
// desired.
template <typename T>
struct FinalizerTrait {
  STATIC_ONLY(FinalizerTrait);
  static const bool kNonTrivialFinalizer =
      WTF::IsSubclassOfTemplate<typename std::remove_const<T>::type,
                                GarbageCollectedFinalized>::value;
  static void Finalize(void* obj) {
    FinalizerTraitImpl<T, kNonTrivialFinalizer>::Finalize(obj);
  }
};

class HeapAllocator;
template <typename T, typename Traits>
class HeapVectorBacking;
template <typename Table>
class HeapHashTableBacking;

template <typename T, typename U, typename V>
struct FinalizerTrait<LinkedHashSet<T, U, V, HeapAllocator>> {
  STATIC_ONLY(FinalizerTrait);
  static const bool kNonTrivialFinalizer = true;
  static void Finalize(void* obj) {
    FinalizerTraitImpl<LinkedHashSet<T, U, V, HeapAllocator>,
                       kNonTrivialFinalizer>::Finalize(obj);
  }
};

template <typename T, typename Allocator>
struct FinalizerTrait<WTF::ListHashSetNode<T, Allocator>> {
  STATIC_ONLY(FinalizerTrait);
  static const bool kNonTrivialFinalizer =
      !WTF::IsTriviallyDestructible<T>::value;
  static void Finalize(void* obj) {
    FinalizerTraitImpl<WTF::ListHashSetNode<T, Allocator>,
                       kNonTrivialFinalizer>::Finalize(obj);
  }
};

template <typename T, size_t inlineCapacity>
struct FinalizerTrait<Vector<T, inlineCapacity, HeapAllocator>> {
  STATIC_ONLY(FinalizerTrait);
  static const bool kNonTrivialFinalizer =
      inlineCapacity && VectorTraits<T>::kNeedsDestruction;
  static void Finalize(void* obj) {
    FinalizerTraitImpl<Vector<T, inlineCapacity, HeapAllocator>,
                       kNonTrivialFinalizer>::Finalize(obj);
  }
};

template <typename T, size_t inlineCapacity>
struct FinalizerTrait<Deque<T, inlineCapacity, HeapAllocator>> {
  STATIC_ONLY(FinalizerTrait);
  static const bool kNonTrivialFinalizer =
      inlineCapacity && VectorTraits<T>::kNeedsDestruction;
  static void Finalize(void* obj) {
    FinalizerTraitImpl<Deque<T, inlineCapacity, HeapAllocator>,
                       kNonTrivialFinalizer>::Finalize(obj);
  }
};

template <typename Table>
struct FinalizerTrait<HeapHashTableBacking<Table>> {
  STATIC_ONLY(FinalizerTrait);
  static const bool kNonTrivialFinalizer =
      !WTF::IsTriviallyDestructible<typename Table::ValueType>::value;
  static void Finalize(void* obj) {
    FinalizerTraitImpl<HeapHashTableBacking<Table>,
                       kNonTrivialFinalizer>::Finalize(obj);
  }
};

template <typename T, typename Traits>
struct FinalizerTrait<HeapVectorBacking<T, Traits>> {
  STATIC_ONLY(FinalizerTrait);
  static const bool kNonTrivialFinalizer = Traits::kNeedsDestruction;
  static void Finalize(void* obj) {
    FinalizerTraitImpl<HeapVectorBacking<T, Traits>,
                       kNonTrivialFinalizer>::Finalize(obj);
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_FINALIZER_TRAITS_H_
