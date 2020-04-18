// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_BLINK_GC_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_BLINK_GC_H_

// BlinkGC.h is a file that defines common things used by Blink GC.

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

#define PRINT_HEAP_STATS 0  // Enable this macro to print heap stats to stderr.

namespace blink {

class HeapObjectHeader;
class MarkingVisitor;
class Visitor;
class ScriptWrappableVisitor;

using Address = uint8_t*;

using FinalizationCallback = void (*)(void*);
using VisitorCallback = void (*)(Visitor*, void*);
using MarkingVisitorCallback = void (*)(MarkingVisitor*, void*);
using TraceCallback = VisitorCallback;
using TraceWrappersCallback = void (*)(ScriptWrappableVisitor*, void*);
using WeakCallback = VisitorCallback;
using EphemeronCallback = VisitorCallback;
using NameCallback = const char* (*)(const void* self);

// Callback used for unit testing the marking of conservative pointers
// (|CheckAndMarkPointer|). For each pointer that has been discovered to point
// to a heap object, the callback is invoked with a pointer to its header. If
// the callback returns true, the object will not be marked.
using MarkedPointerCallbackForTesting = bool (*)(HeapObjectHeader*);

// Simple alias to avoid heap compaction type signatures turning into
// a sea of generic |void*|s.
using MovableReference = void*;

// Heap compaction supports registering callbacks that are to be invoked
// when an object is moved during compaction. This is to support internal
// location fixups that need to happen as a result.
//
// i.e., when the object residing at |from| is moved to |to| by the compaction
// pass, invoke the callback to adjust any internal references that now need
// to be |to|-relative.
using MovingObjectCallback = void (*)(void* callback_data,
                                      MovableReference from,
                                      MovableReference to,
                                      size_t);

// List of typed arenas. The list is used to generate the implementation
// of typed arena related methods.
//
// To create a new typed arena add a H(<ClassName>) to the
// FOR_EACH_TYPED_ARENA macro below.
#define FOR_EACH_TYPED_ARENA(H) \
  H(Node)                       \
  H(CSSValue)

#define TypedArenaEnumName(Type) k##Type##ArenaIndex,

class PLATFORM_EXPORT WorklistTaskId {
 public:
  static constexpr int MainThread = 0;
};

class PLATFORM_EXPORT BlinkGC final {
  STATIC_ONLY(BlinkGC);

 public:
  // When garbage collecting we need to know whether or not there
  // can be pointers to Blink GC managed objects on the stack for
  // each thread. When threads reach a safe point they record
  // whether or not they have pointers on the stack.
  enum StackState { kNoHeapPointersOnStack, kHeapPointersOnStack };

  enum MarkingType {
    // The marking completes synchronously.
    kAtomicMarking,
    // The marking task is split and executed in chunks.
    kIncrementalMarking,
    // We run marking to take a heap snapshot. Sweeping should do nothing and
    // just clear the mark flags.
    kTakeSnapshot,
  };

  enum SweepingType {
    // The sweeping task is split into chunks and scheduled lazily.
    kLazySweeping,
    // The sweeping task executs synchronously right after marking.
    kEagerSweeping,
  };

  enum GCReason {
    kIdleGC = 0,
    kPreciseGC = 1,
    kConservativeGC = 2,
    kForcedGC = 3,
    kMemoryPressureGC = 4,
    kPageNavigationGC = 5,
    kThreadTerminationGC = 6,
    kTesting = 7,
    kLastGCReason = kTesting,
  };

  enum ArenaIndices {
    kEagerSweepArenaIndex = 0,
    kNormalPage1ArenaIndex,
    kNormalPage2ArenaIndex,
    kNormalPage3ArenaIndex,
    kNormalPage4ArenaIndex,
    kVector1ArenaIndex,
    kVector2ArenaIndex,
    kVector3ArenaIndex,
    kVector4ArenaIndex,
    kInlineVectorArenaIndex,
    kHashTableArenaIndex,
    FOR_EACH_TYPED_ARENA(TypedArenaEnumName) kLargeObjectArenaIndex,
    // Values used for iteration of heap segments.
    kNumberOfArenas,
  };

  enum V8GCType {
    kV8MinorGC,
    kV8MajorGC,
  };

  // Sentinel used to mark not-fully-constructed during mixins.
  static constexpr void* kNotFullyConstructedObject = nullptr;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_BLINK_GC_H_
