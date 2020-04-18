// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/exclusions/ng_exclusion_space.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

#define TEST_OPPORTUNITY(opp, expected_start_offset, expected_end_offset) \
  EXPECT_EQ(expected_start_offset, opp.rect.start_offset);                \
  EXPECT_EQ(expected_end_offset, opp.rect.end_offset)

// Tests that an empty exclusion space returns exactly one layout opportunity
// each one, and sized appropriately given the area.
TEST(NGExclusionSpaceTest, Empty) {
  NGExclusionSpace exclusion_space;

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(100));

  EXPECT_EQ(1u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));

  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(-30), LayoutUnit(-100)},
      /* available_size */ LayoutUnit(50));

  EXPECT_EQ(1u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0],
                   NGBfcOffset(LayoutUnit(-30), LayoutUnit(-100)),
                   NGBfcOffset(LayoutUnit(20), LayoutUnit::Max()));

  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(30), LayoutUnit(100)},
      /* available_size */ LayoutUnit(50));

  EXPECT_EQ(1u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0],
                   NGBfcOffset(LayoutUnit(30), LayoutUnit(100)),
                   NGBfcOffset(LayoutUnit(80), LayoutUnit::Max()));
}

TEST(NGExclusionSpaceTest, SingleExclusion) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(10), LayoutUnit(20)),
                NGBfcOffset(LayoutUnit(60), LayoutUnit(90))),
      EFloat::kLeft));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(100));

  EXPECT_EQ(3u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit(20)));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(60), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(), LayoutUnit(90)),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));

  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(-10), LayoutUnit(-100)},
      /* available_size */ LayoutUnit(100));

  EXPECT_EQ(3u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0],
                   NGBfcOffset(LayoutUnit(-10), LayoutUnit(-100)),
                   NGBfcOffset(LayoutUnit(90), LayoutUnit(20)));
  TEST_OPPORTUNITY(opportunites[1],
                   NGBfcOffset(LayoutUnit(60), LayoutUnit(-100)),
                   NGBfcOffset(LayoutUnit(90), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2],
                   NGBfcOffset(LayoutUnit(-10), LayoutUnit(90)),
                   NGBfcOffset(LayoutUnit(90), LayoutUnit::Max()));

  // This will still produce three opportunities, with a zero-width RHS
  // opportunity.
  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(10), LayoutUnit(10)},
      /* available_size */ LayoutUnit(50));

  EXPECT_EQ(3u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(10), LayoutUnit(10)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit(20)));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(60), LayoutUnit(10)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(10), LayoutUnit(90)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit::Max()));

  // This will also produce three opportunities, as the RHS opportunity outside
  // the search area creates a zero-width opportunity.
  opportunites = exclusion_space.AllLayoutOpportunities(
      /* offset */ {LayoutUnit(10), LayoutUnit(10)},
      /* available_size */ LayoutUnit(49));

  EXPECT_EQ(3u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(10), LayoutUnit(10)),
                   NGBfcOffset(LayoutUnit(59), LayoutUnit(20)));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(60), LayoutUnit(10)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(10), LayoutUnit(90)),
                   NGBfcOffset(LayoutUnit(59), LayoutUnit::Max()));
}

TEST(NGExclusionSpaceTest, TwoExclusions) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(), LayoutUnit()),
                NGBfcOffset(LayoutUnit(150), LayoutUnit(75))),
      EFloat::kLeft));
  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(100), LayoutUnit(75)),
                NGBfcOffset(LayoutUnit(400), LayoutUnit(150))),
      EFloat::kRight));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(400));

  EXPECT_EQ(4u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(150), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(400), LayoutUnit(75)));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(150), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(150), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(), LayoutUnit(75)),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[3], NGBfcOffset(LayoutUnit(), LayoutUnit(150)),
                   NGBfcOffset(LayoutUnit(400), LayoutUnit::Max()));
}

// Tests the "solid edge" behaviour. When "NEW" is added a new layout
// opportunity shouldn't be created above it.
//
// NOTE: This is the same example given in the code.
//
//    0 1 2 3 4 5 6 7 8
// 0  +---+  X----X+---+
//    |xxx|  .     |xxx|
// 10 |xxx|  .     |xxx|
//    +---+  .     +---+
// 20        .     .
//      +---+. .+---+
// 30   |xxx|   |NEW|
//      |xxx|   +---+
// 40   +---+
TEST(NGExclusionSpaceTest, SolidEdges) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(), LayoutUnit()),
                NGBfcOffset(LayoutUnit(20), LayoutUnit(15))),
      EFloat::kLeft));
  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(65), LayoutUnit()),
                NGBfcOffset(LayoutUnit(85), LayoutUnit(15))),
      EFloat::kRight));
  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(10), LayoutUnit(25)),
                NGBfcOffset(LayoutUnit(30), LayoutUnit(40))),
      EFloat::kLeft));
  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(50), LayoutUnit(25)),
                NGBfcOffset(LayoutUnit(70), LayoutUnit(35))),
      EFloat::kRight));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(80));

  EXPECT_EQ(5u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(20), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(65), LayoutUnit(25)));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(30), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(50), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(), LayoutUnit(15)),
                   NGBfcOffset(LayoutUnit(80), LayoutUnit(25)));
  TEST_OPPORTUNITY(opportunites[3], NGBfcOffset(LayoutUnit(30), LayoutUnit(35)),
                   NGBfcOffset(LayoutUnit(80), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[4], NGBfcOffset(LayoutUnit(), LayoutUnit(40)),
                   NGBfcOffset(LayoutUnit(80), LayoutUnit::Max()));
}

// Tests that if a new exclusion doesn't overlap with a shelf, we don't add a
// new layout opportunity.
//
// NOTE: This is the same example given in the code.
//
//    0 1 2 3 4 5 6 7 8
// 0  +---+X------X+---+
//    |xxx|        |xxx|
// 10 |xxx|        |xxx|
//    +---+        +---+
// 20
//                  +---+
// 30               |NEW|
//                  +---+
TEST(NGExclusionSpaceTest, OverlappingWithShelf) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(), LayoutUnit()),
                NGBfcOffset(LayoutUnit(20), LayoutUnit(15))),
      EFloat::kLeft));
  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(65), LayoutUnit()),
                NGBfcOffset(LayoutUnit(85), LayoutUnit(15))),
      EFloat::kRight));
  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(70), LayoutUnit(25)),
                NGBfcOffset(LayoutUnit(90), LayoutUnit(35))),
      EFloat::kRight));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(80));

  EXPECT_EQ(4u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(20), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(65), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(), LayoutUnit(15)),
                   NGBfcOffset(LayoutUnit(80), LayoutUnit(25)));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(), LayoutUnit(15)),
                   NGBfcOffset(LayoutUnit(70), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[3], NGBfcOffset(LayoutUnit(), LayoutUnit(35)),
                   NGBfcOffset(LayoutUnit(80), LayoutUnit::Max()));
}

// Tests that a shelf is properly inserted between two other shelves.
//
// Additionally tests that an inserted exclusion is correctly inserted in a
// shelve's line_left_edges/line_right_edges list.
//
// NOTE: This is the same example given in the code.
//
//    0 1 2 3 4 5 6 7 8
// 0  +-----+X----X+---+
//    |xxxxx|      |xxx|
// 10 +-----+      |xxx|
//      +---+      |xxx|
// 20   |NEW|      |xxx|
//    X-----------X|xxx|
// 30              |xxx|
//    X----------------X
TEST(NGExclusionSpaceTest, InsertBetweenShelves) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(), LayoutUnit()),
                NGBfcOffset(LayoutUnit(30), LayoutUnit(10))),
      EFloat::kLeft));
  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(65), LayoutUnit()),
                NGBfcOffset(LayoutUnit(85), LayoutUnit(35))),
      EFloat::kRight));
  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(10), LayoutUnit(15)),
                NGBfcOffset(LayoutUnit(30), LayoutUnit(25))),
      EFloat::kLeft));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(30), LayoutUnit(15)},
          /* available_size */ LayoutUnit(30));

  // NOTE: This demonstrates a quirk when querying the exclusion space for
  // opportunities. The exclusion space may return multiple exclusions of
  // exactly the same (or growing) size. This quirk still produces correct
  // results for code which uses it as the exclusions grow or keep the same
  // size.
  EXPECT_EQ(3u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(30), LayoutUnit(15)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(30), LayoutUnit(25)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[2], NGBfcOffset(LayoutUnit(30), LayoutUnit(35)),
                   NGBfcOffset(LayoutUnit(60), LayoutUnit::Max()));
}

TEST(NGExclusionSpaceTest, ZeroInlineSizeOpportunity) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(), LayoutUnit()),
                NGBfcOffset(LayoutUnit(100), LayoutUnit(10))),
      EFloat::kLeft));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(100));

  EXPECT_EQ(2u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(100), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(), LayoutUnit(10)),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));
}

TEST(NGExclusionSpaceTest, NegativeInlineSizeOpportunityLeft) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(), LayoutUnit()),
                NGBfcOffset(LayoutUnit(120), LayoutUnit(10))),
      EFloat::kLeft));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(100));

  EXPECT_EQ(2u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(120), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(120), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(), LayoutUnit(10)),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));
}

TEST(NGExclusionSpaceTest, NegativeInlineSizeOpportunityRight) {
  NGExclusionSpace exclusion_space;

  exclusion_space.Add(NGExclusion::Create(
      NGBfcRect(NGBfcOffset(LayoutUnit(-20), LayoutUnit()),
                NGBfcOffset(LayoutUnit(100), LayoutUnit(10))),
      EFloat::kRight));

  Vector<NGLayoutOpportunity> opportunites =
      exclusion_space.AllLayoutOpportunities(
          /* offset */ {LayoutUnit(), LayoutUnit()},
          /* available_size */ LayoutUnit(100));

  EXPECT_EQ(2u, opportunites.size());
  TEST_OPPORTUNITY(opportunites[0], NGBfcOffset(LayoutUnit(), LayoutUnit()),
                   NGBfcOffset(LayoutUnit(), LayoutUnit::Max()));
  TEST_OPPORTUNITY(opportunites[1], NGBfcOffset(LayoutUnit(), LayoutUnit(10)),
                   NGBfcOffset(LayoutUnit(100), LayoutUnit::Max()));
}

}  // namespace
}  // namespace blink
