// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/enqueue_order.h"

namespace base {
namespace sequence_manager {
namespace internal {

// Note we set the first |enqueue_order_| to a specific non-zero value, because
// first N values of EnqueueOrder have special meaning (see EnqueueOrderValues).
EnqueueOrderGenerator::EnqueueOrderGenerator()
    : enqueue_order_(static_cast<EnqueueOrder>(EnqueueOrderValues::kFirst)) {}

EnqueueOrderGenerator::~EnqueueOrderGenerator() = default;

EnqueueOrder EnqueueOrderGenerator::GenerateNext() {
  AutoLock lock(lock_);
  return enqueue_order_++;
}

}  // namespace internal
}  // namespace sequence_manager
}  // namespace base
