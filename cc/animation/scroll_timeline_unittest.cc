// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/scroll_timeline.h"
#include "cc/trees/property_tree.h"
#include "cc/trees/scroll_node.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/scroll_offset.h"

namespace cc {

class ScrollTimelineTest : public ::testing::Test {
 public:
  ScrollTimelineTest()
      : scroller_id_(1), container_size_(100, 100), content_size_(500, 500) {
    // For simplicity we make the property_tree main thread; this avoids the
    // need to deal with the synced scroll offset code.
    property_trees_.is_main_thread = true;
    property_trees_.is_active = true;

    // We add a single node that is scrolling a 550x1100 contents inside a
    // 50x100 container.
    ScrollNode node;
    node.scrollable = true;
    node.bounds = content_size_;
    node.container_bounds = container_size_;

    int node_id = property_trees_.scroll_tree.Insert(node, 0);
    property_trees_.element_id_to_scroll_node_index[scroller_id_] = node_id;
  }

  ScrollTree& scroll_tree() { return property_trees_.scroll_tree; }
  ElementId scroller_id() const { return scroller_id_; }
  gfx::Size container_size() const { return container_size_; }
  gfx::Size content_size() const { return content_size_; }

 private:
  PropertyTrees property_trees_;
  ElementId scroller_id_;
  gfx::Size container_size_;
  gfx::Size content_size_;
};

TEST_F(ScrollTimelineTest, BasicCurrentTimeCalculations) {
  // For simplicity, we set the time range such that the current time maps
  // directly to the scroll offset. We have a square scroller/contents, so can
  // just compute one edge and use it for vertical/horizontal.
  double time_range = content_size().height() - container_size().height();

  ScrollTimeline vertical_timeline(scroller_id(), ScrollTimeline::Vertical,
                                   time_range);
  ScrollTimeline horizontal_timeline(scroller_id(), ScrollTimeline::Horizontal,
                                     time_range);

  // Unscrolled, both timelines should read a current time of 0.
  scroll_tree().SetScrollOffset(scroller_id(), gfx::ScrollOffset());
  EXPECT_FLOAT_EQ(0, vertical_timeline.CurrentTime(scroll_tree()));
  EXPECT_FLOAT_EQ(0, horizontal_timeline.CurrentTime(scroll_tree()));

  // Now do some scrolling and make sure that the ScrollTimelines update.
  scroll_tree().SetScrollOffset(scroller_id(), gfx::ScrollOffset(75, 50));

  // As noted above, we have mapped the time range such that current time should
  // just be the scroll offset.
  EXPECT_FLOAT_EQ(50, vertical_timeline.CurrentTime(scroll_tree()));
  EXPECT_FLOAT_EQ(75, horizontal_timeline.CurrentTime(scroll_tree()));
}

TEST_F(ScrollTimelineTest, CurrentTimeIsAdjustedForTimeRange) {
  // Here we set a time range to 100, which gives the current time the form of
  // 'percentage scrolled'.
  ScrollTimeline timeline(scroller_id(), ScrollTimeline::Vertical, 100);

  double halfwayY = (content_size().height() - container_size().height()) / 2.;
  scroll_tree().SetScrollOffset(scroller_id(), gfx::ScrollOffset(0, halfwayY));

  EXPECT_FLOAT_EQ(50, timeline.CurrentTime(scroll_tree()));
}

}  // namespace cc
