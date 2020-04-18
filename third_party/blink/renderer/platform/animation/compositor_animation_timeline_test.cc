// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/animation/compositor_animation_timeline.h"

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "cc/animation/animation_host.h"
#include "third_party/blink/renderer/platform/animation/compositor_animation.h"
#include "third_party/blink/renderer/platform/animation/compositor_animation_host.h"
#include "third_party/blink/renderer/platform/testing/compositor_test.h"
#include "third_party/blink/renderer/platform/testing/web_layer_tree_view_impl_for_testing.h"

namespace blink {

class CompositorAnimationTimelineTest : public CompositorTest {};

TEST_F(CompositorAnimationTimelineTest,
       CompositorTimelineDeletionDetachesFromAnimationHost) {
  std::unique_ptr<CompositorAnimationTimeline> timeline =
      CompositorAnimationTimeline::Create();

  scoped_refptr<cc::AnimationTimeline> cc_timeline =
      timeline->GetAnimationTimeline();
  EXPECT_FALSE(cc_timeline->animation_host());

  WebLayerTreeViewImplForTesting layer_tree_view;
  CompositorAnimationHost compositor_animation_host(
      layer_tree_view.CompositorAnimationHost());

  compositor_animation_host.AddTimeline(*timeline);
  cc::AnimationHost* animation_host = cc_timeline->animation_host();
  EXPECT_TRUE(animation_host);
  EXPECT_TRUE(animation_host->GetTimelineById(cc_timeline->id()));

  // Delete CompositorAnimationTimeline while attached to host.
  timeline = nullptr;

  EXPECT_FALSE(cc_timeline->animation_host());
  EXPECT_FALSE(animation_host->GetTimelineById(cc_timeline->id()));
}

}  // namespace blink
