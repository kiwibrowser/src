// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_offset_rect.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"

namespace blink {

namespace {

struct PhysicalOffsetRectUniteTestData {
  NGPhysicalOffsetRect a;
  NGPhysicalOffsetRect b;
  NGPhysicalOffsetRect expected;
} physical_offset_rect_unite_test_data[] = {
    {{}, {}, {}},
    {{},
     {{LayoutUnit(1), LayoutUnit(2)}, {LayoutUnit(3), LayoutUnit(4)}},
     {{LayoutUnit(1), LayoutUnit(2)}, {LayoutUnit(3), LayoutUnit(4)}}},
    {{{LayoutUnit(1), LayoutUnit(2)}, {LayoutUnit(3), LayoutUnit(4)}},
     {},
     {{LayoutUnit(1), LayoutUnit(2)}, {LayoutUnit(3), LayoutUnit(4)}}},
    {{{LayoutUnit(100), LayoutUnit(50)}, {LayoutUnit(300), LayoutUnit(200)}},
     {{LayoutUnit(200), LayoutUnit(50)}, {LayoutUnit(200), LayoutUnit(200)}},
     {{LayoutUnit(100), LayoutUnit(50)}, {LayoutUnit(300), LayoutUnit(200)}}},
    {{{LayoutUnit(200), LayoutUnit(50)}, {LayoutUnit(200), LayoutUnit(200)}},
     {{LayoutUnit(100), LayoutUnit(50)}, {LayoutUnit(300), LayoutUnit(200)}},
     {{LayoutUnit(100), LayoutUnit(50)}, {LayoutUnit(300), LayoutUnit(200)}}},
};

std::ostream& operator<<(std::ostream& os,
                         const PhysicalOffsetRectUniteTestData& data) {
  WTF::Partitions::Initialize(nullptr);
  return os << "Unite " << data.a << " and " << data.b;
}

class NGPhysicalOffsetRectUniteTest
    : public testing::Test,
      public testing::WithParamInterface<PhysicalOffsetRectUniteTestData> {};

INSTANTIATE_TEST_CASE_P(
    NGGeometryUnitsTest,
    NGPhysicalOffsetRectUniteTest,
    testing::ValuesIn(physical_offset_rect_unite_test_data));

TEST_P(NGPhysicalOffsetRectUniteTest, Data) {
  const auto& data = GetParam();
  NGPhysicalOffsetRect actual = data.a;
  actual.Unite(data.b);
  EXPECT_EQ(data.expected, actual);
}

}  // namespace

}  // namespace blink
