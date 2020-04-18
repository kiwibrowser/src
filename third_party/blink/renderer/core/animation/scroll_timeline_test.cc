// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/scroll_timeline.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

namespace blink {

using ScrollTimelineTest = RenderingTest;

TEST_F(ScrollTimelineTest,
       AttachingAndDetachingAnimationCausesCompositingUpdate) {
  EnableCompositing();

  SetBodyInnerHTML(R"HTML(
    <style>#scroller { overflow: scroll; width: 100px; height: 100px; }</style>
    <div id='scroller'></div>
  )HTML");

  LayoutBoxModelObject* scroller =
      ToLayoutBoxModelObject(GetLayoutObjectByElementId("scroller"));
  ASSERT_TRUE(scroller);

  // Invariant: the scroller is not composited by default.
  EXPECT_EQ(DocumentLifecycle::kPaintClean,
            GetDocument().Lifecycle().GetState());
  EXPECT_EQ(kNotComposited, scroller->Layer()->GetCompositingState());

  // Create the ScrollTimeline. This shouldn't cause the scrollSource to need
  // compositing, as it isn't attached to any animation yet.
  ScrollTimelineOptions options;
  DoubleOrScrollTimelineAutoKeyword time_range =
      DoubleOrScrollTimelineAutoKeyword::FromDouble(100);
  options.setTimeRange(time_range);
  options.setScrollSource(GetElementById("scroller"));
  ScrollTimeline* scroll_timeline =
      ScrollTimeline::Create(GetDocument(), options, ASSERT_NO_EXCEPTION);
  EXPECT_EQ(DocumentLifecycle::kPaintClean,
            GetDocument().Lifecycle().GetState());
  EXPECT_EQ(kNotComposited, scroller->Layer()->GetCompositingState());

  // Now attach an animation. This should require a compositing update.
  scroll_timeline->AttachAnimation();

  UpdateAllLifecyclePhases();
  EXPECT_NE(scroller->Layer()->GetCompositingState(), kNotComposited);

  // Now detach an animation. This should again require a compositing update.
  scroll_timeline->DetachAnimation();

  UpdateAllLifecyclePhases();
  EXPECT_EQ(scroller->Layer()->GetCompositingState(), kNotComposited);
}

}  //  namespace blink
