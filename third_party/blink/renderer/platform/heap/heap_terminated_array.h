// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TERMINATED_ARRAY_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TERMINATED_ARRAY_H_

#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/construct_traits.h"
#include "third_party/blink/renderer/platform/wtf/terminated_array.h"
#include "third_party/blink/renderer/platform/wtf/terminated_array_builder.h"

namespace blink {

template <typename T>
class HeapTerminatedArray : public TerminatedArray<T> {
  DISALLOW_NEW();
  IS_GARBAGE_COLLECTED_TYPE();

 public:
  using TerminatedArray<T>::begin;
  using TerminatedArray<T>::end;

  void Trace(blink::Visitor* visitor) {
    for (typename TerminatedArray<T>::iterator it = begin(); it != end(); ++it)
      visitor->Trace(*it);
  }

 private:
  // Allocator describes how HeapTerminatedArrayBuilder should create new
  // instances of HeapTerminatedArray and manage their lifetimes.
  struct Allocator final {
    STATIC_ONLY(Allocator);
    using BackendAllocator = HeapAllocator;
    using PassPtr = HeapTerminatedArray*;
    using Ptr = Member<HeapTerminatedArray>;

    static PassPtr Release(Ptr& ptr) { return ptr; }

    static PassPtr Create(size_t capacity) {
      // No ConstructTraits as there are no real elements in the array after
      // construction.
      return reinterpret_cast<HeapTerminatedArray*>(
          ThreadHeap::Allocate<HeapTerminatedArray>(
              WTF::Partitions::ComputeAllocationSize(capacity, sizeof(T)),
              IsEagerlyFinalizedType<T>::value));
    }

    static PassPtr Resize(PassPtr ptr, size_t capacity) {
      PassPtr array = reinterpret_cast<HeapTerminatedArray*>(
          ThreadHeap::Reallocate<HeapTerminatedArray>(
              ptr,
              WTF::Partitions::ComputeAllocationSize(capacity, sizeof(T))));
      WTF::ConstructTraits<T, VectorTraits<T>, HeapAllocator>::
          NotifyNewElements(reinterpret_cast<T*>(array), array->size());
      return array;
    }
  };

  // Prohibit construction. Allocator makes HeapTerminatedArray instances for
  // HeapTerminatedArrayBuilder by pointer casting.
  HeapTerminatedArray() = delete;

  template <typename U, template <typename> class>
  friend class WTF::TerminatedArrayBuilder;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TERMINATED_ARRAY_H_
