// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/animation_host.h"

#include "cc/animation/animation_id_provider.h"
#include "cc/animation/animation_timeline.h"
#include "cc/test/animation_test_common.h"
#include "cc/test/animation_timelines_test_common.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class AnimationHostTest : public AnimationTimelinesTest {
 public:
  AnimationHostTest() = default;
  ~AnimationHostTest() override = default;
};

// See Animation tests on layer registration/unregistration in
// animation_unittest.cc.

TEST_F(AnimationHostTest, SyncTimelinesAddRemove) {
  std::unique_ptr<AnimationHost> host(
      AnimationHost::CreateForTesting(ThreadInstance::MAIN));
  std::unique_ptr<AnimationHost> host_impl(
      AnimationHost::CreateForTesting(ThreadInstance::IMPL));

  const int timeline_id = AnimationIdProvider::NextTimelineId();
  scoped_refptr<AnimationTimeline> timeline(
      AnimationTimeline::Create(timeline_id));
  host->AddAnimationTimeline(timeline.get());
  EXPECT_TRUE(timeline->animation_host());

  EXPECT_FALSE(host_impl->GetTimelineById(timeline_id));

  host->PushPropertiesTo(host_impl.get());

  scoped_refptr<AnimationTimeline> timeline_impl =
      host_impl->GetTimelineById(timeline_id);
  EXPECT_TRUE(timeline_impl);
  EXPECT_EQ(timeline_impl->id(), timeline_id);

  host->PushPropertiesTo(host_impl.get());
  EXPECT_EQ(timeline_impl, host_impl->GetTimelineById(timeline_id));

  host->RemoveAnimationTimeline(timeline.get());
  EXPECT_FALSE(timeline->animation_host());

  host->PushPropertiesTo(host_impl.get());
  EXPECT_FALSE(host_impl->GetTimelineById(timeline_id));

  EXPECT_FALSE(timeline_impl->animation_host());
}

TEST_F(AnimationHostTest, ImplOnlyTimeline) {
  std::unique_ptr<AnimationHost> host(
      AnimationHost::CreateForTesting(ThreadInstance::MAIN));
  std::unique_ptr<AnimationHost> host_impl(
      AnimationHost::CreateForTesting(ThreadInstance::IMPL));

  const int timeline_id1 = AnimationIdProvider::NextTimelineId();
  const int timeline_id2 = AnimationIdProvider::NextTimelineId();

  scoped_refptr<AnimationTimeline> timeline(
      AnimationTimeline::Create(timeline_id1));
  scoped_refptr<AnimationTimeline> timeline_impl(
      AnimationTimeline::Create(timeline_id2));
  timeline_impl->set_is_impl_only(true);

  host->AddAnimationTimeline(timeline.get());
  host_impl->AddAnimationTimeline(timeline_impl.get());

  host->PushPropertiesTo(host_impl.get());

  EXPECT_TRUE(host->GetTimelineById(timeline_id1));
  EXPECT_TRUE(host_impl->GetTimelineById(timeline_id2));
}

TEST_F(AnimationHostTest, ImplOnlyScrollAnimationUpdateTargetIfDetached) {
  client_.RegisterElement(element_id_, ElementListType::ACTIVE);
  client_impl_.RegisterElement(element_id_, ElementListType::PENDING);

  gfx::ScrollOffset target_offset(0., 2.);
  gfx::ScrollOffset current_offset(0., 1.);
  host_impl_->ImplOnlyScrollAnimationCreate(element_id_, target_offset,
                                            current_offset, base::TimeDelta(),
                                            base::TimeDelta());

  gfx::Vector2dF scroll_delta(0, 0.5);
  gfx::ScrollOffset max_scroll_offset(0., 3.);

  base::TimeTicks time;

  time += base::TimeDelta::FromSecondsD(0.1);
  EXPECT_TRUE(host_impl_->ImplOnlyScrollAnimationUpdateTarget(
      element_id_, scroll_delta, max_scroll_offset, time, base::TimeDelta()));

  // Detach all animations from layers and timelines.
  host_impl_->ClearMutators();

  time += base::TimeDelta::FromSecondsD(0.1);
  EXPECT_FALSE(host_impl_->ImplOnlyScrollAnimationUpdateTarget(
      element_id_, scroll_delta, max_scroll_offset, time, base::TimeDelta()));
}

}  // namespace
}  // namespace cc
