// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint_invalidation_reason.h"

#include <sstream>
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(PaintInvalidationReasonTest, StreamOutput) {
  {
    std::stringstream string_stream;
    PaintInvalidationReason reason = PaintInvalidationReason::kNone;
    string_stream << reason;
    EXPECT_EQ("none", string_stream.str());
  }
  {
    std::stringstream string_stream;
    PaintInvalidationReason reason = PaintInvalidationReason::kDelayedFull;
    string_stream << reason;
    EXPECT_EQ("delayed full", string_stream.str());
  }
}

}  // namespace blink
