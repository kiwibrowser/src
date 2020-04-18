// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TERMINATED_ARRAY_BUILDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TERMINATED_ARRAY_BUILDER_H_

#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/heap/heap_terminated_array.h"
#include "third_party/blink/renderer/platform/wtf/terminated_array_builder.h"

namespace blink {

template <typename T>
class HeapTerminatedArrayBuilder final
    : public TerminatedArrayBuilder<T, HeapTerminatedArray> {
  STACK_ALLOCATED();

 public:
  explicit HeapTerminatedArrayBuilder(HeapTerminatedArray<T>* array)
      : TerminatedArrayBuilder<T, HeapTerminatedArray>(array) {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_TERMINATED_ARRAY_BUILDER_H_
