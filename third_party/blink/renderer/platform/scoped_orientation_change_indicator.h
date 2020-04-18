// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCOPED_ORIENTATION_CHANGE_INDICATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCOPED_ORIENTATION_CHANGE_INDICATOR_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class PLATFORM_EXPORT ScopedOrientationChangeIndicator final {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(ScopedOrientationChangeIndicator);

 public:
  static bool ProcessingOrientationChange();

  explicit ScopedOrientationChangeIndicator();
  ~ScopedOrientationChangeIndicator();

 private:
  enum class State {
    kProcessing,
    kNotProcessing,
  };

  static State state_;

  State previous_state_ = State::kNotProcessing;
};

}  // namespace blink

#endif
