/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "third_party/blink/renderer/platform/heap/heap_test_utilities.h"

#include "third_party/blink/renderer/platform/heap/heap.h"

namespace blink {

void PreciselyCollectGarbage() {
  ThreadState::Current()->CollectGarbage(
      BlinkGC::kNoHeapPointersOnStack, BlinkGC::kAtomicMarking,
      BlinkGC::kEagerSweeping, BlinkGC::kForcedGC);
}

void ConservativelyCollectGarbage() {
  ThreadState::Current()->CollectGarbage(
      BlinkGC::kHeapPointersOnStack, BlinkGC::kAtomicMarking,
      BlinkGC::kEagerSweeping, BlinkGC::kForcedGC);
}

// Do several GCs to make sure that later GCs don't free up old memory from
// previously run tests in this process.
void ClearOutOldGarbage() {
  ThreadHeap& heap = ThreadState::Current()->Heap();
  while (true) {
    size_t used = heap.ObjectPayloadSizeForTesting();
    PreciselyCollectGarbage();
    if (heap.ObjectPayloadSizeForTesting() >= used)
      break;
  }
}

}  // namespace blink
