// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_STACK_FRAME_DEPTH_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_STACK_FRAME_DEPTH_H_

#include <stdint.h>
#include <cstddef>
#include "base/macros.h"
#include "build/build_config.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

// StackFrameDepth keeps track of current call stack frame depth.
// It is specifically used to control stack usage while tracing
// the object graph during a GC.
//
// Use isSafeToRecurse() to determine if it is safe to consume
// more stack by invoking another recursive call.
class PLATFORM_EXPORT StackFrameDepth final {
  DISALLOW_NEW();

 public:
  StackFrameDepth() : stack_frame_limit_(kMinimumStackLimit) {}
  bool IsSafeToRecurse() {
    // Asssume that the stack grows towards lower addresses, which
    // all the ABIs currently supported do.
    //
    // A unit test checks that the assumption holds for a target
    // (HeapTest.StackGrowthDirection.)
    return CurrentStackFrame() > stack_frame_limit_;
  }

  void EnableStackLimit();
  void DisableStackLimit() { stack_frame_limit_ = kMinimumStackLimit; }

  bool IsEnabled() { return stack_frame_limit_ != kMinimumStackLimit; }
  bool IsAcceptableStackUse() {
#if defined(ADDRESS_SANITIZER)
    // ASan adds extra stack usage leading to too noisy asserts.
    return true;
#else
    return !IsEnabled() || IsSafeToRecurse();
#endif
  }

#if defined(COMPILER_MSVC)
// Ignore C4172: returning address of local variable or temporary: dummy. This
// warning suppression has to go outside of the function to take effect.
#pragma warning(push)
#pragma warning(disable : 4172)
#endif
  static uintptr_t CurrentStackFrame(const char* dummy = nullptr) {
#if defined(COMPILER_GCC)
    return reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
#elif defined(COMPILER_MSVC)
    return reinterpret_cast<uintptr_t>(&dummy) - sizeof(void*);
#else
#error "Stack frame pointer estimation not supported on this platform."
    return 0;
#endif
  }
#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif

 private:
  // The maximum depth of eager, unrolled trace() calls that is
  // considered safe and allowed for targets with an unknown
  // thread stack size.
  static const int kSafeStackFrameSize = 32 * 1024;

  // The stack pointer is assumed to grow towards lower addresses;
  // |kMinimumStackLimit| then being the limit that a stack
  // pointer will always exceed.
  static const uintptr_t kMinimumStackLimit = ~uintptr_t{0};

  static uintptr_t GetFallbackStackLimit();

  // The (pointer-valued) stack limit.
  uintptr_t stack_frame_limit_;
};

class StackFrameDepthScope {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(StackFrameDepthScope);

 public:
  explicit StackFrameDepthScope(StackFrameDepth* depth) : depth_(depth) {
    depth_->EnableStackLimit();
    // Enabled unless under stack pressure.
    DCHECK(depth_->IsSafeToRecurse() || !depth_->IsEnabled());
  }

  ~StackFrameDepthScope() { depth_->DisableStackLimit(); }

 private:
  StackFrameDepth* depth_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_STACK_FRAME_DEPTH_H_
