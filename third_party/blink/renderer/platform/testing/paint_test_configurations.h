// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_PAINT_TEST_CONFIGURATIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_PAINT_TEST_CONFIGURATIONS_H_

#include <gtest/gtest.h>
#include "third_party/blink/public/web/web_heap.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

enum {
  kSlimmingPaintV175 = 1 << 0,
  kBlinkGenPropertyTrees = 1 << 1,
  kSlimmingPaintV2 = 1 << 2,
  kUnderInvalidationChecking = 1 << 3,
};

class PaintTestConfigurations
    : public testing::WithParamInterface<unsigned>,
      private ScopedSlimmingPaintV175ForTest,
      private ScopedBlinkGenPropertyTreesForTest,
      private ScopedSlimmingPaintV2ForTest,
      private ScopedPaintUnderInvalidationCheckingForTest {
 public:
  PaintTestConfigurations()
      : ScopedSlimmingPaintV175ForTest(GetParam() & kSlimmingPaintV175),
        ScopedBlinkGenPropertyTreesForTest(GetParam() & kBlinkGenPropertyTrees),
        ScopedSlimmingPaintV2ForTest(GetParam() & kSlimmingPaintV2),
        ScopedPaintUnderInvalidationCheckingForTest(
            GetParam() & kUnderInvalidationChecking) {}
  ~PaintTestConfigurations() {
    // Must destruct all objects before toggling back feature flags.
    WebHeap::CollectAllGarbageForTesting();
  }
};

#define INSTANTIATE_PAINT_TEST_CASE_P(test_class)                      \
  INSTANTIATE_TEST_CASE_P(                                             \
      All, test_class,                                                 \
      ::testing::Values(0, kSlimmingPaintV175, kBlinkGenPropertyTrees, \
                        kSlimmingPaintV2))

#define INSTANTIATE_SPV2_TEST_CASE_P(test_class) \
  INSTANTIATE_TEST_CASE_P(All, test_class, ::testing::Values(kSlimmingPaintV2))

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_PAINT_TEST_CONFIGURATIONS_H_
