// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/transient_element.h"

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "chrome/browser/vr/test/animation_utils.h"
#include "chrome/browser/vr/test/constants.h"
#include "chrome/browser/vr/ui_scene.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

namespace {

bool DoBeginFrame(UiElement* element, int time_milliseconds) {
  element->set_last_frame_time(MsToTicks(time_milliseconds));
  const bool force_animations_to_completion = false;
  return element->DoBeginFrame(kStartHeadPose, force_animations_to_completion);
}

}  // namespace

TEST(SimpleTransientElementTest, Visibility) {
  SimpleTransientElement element(base::TimeDelta::FromSeconds(2));
  element.SetOpacity(0.0f);

  EXPECT_EQ(0.0f, element.opacity());

  // Enable and disable, making sure the element appears and disappears.
  element.SetVisible(true);
  EXPECT_EQ(element.opacity_when_visible(), element.opacity());
  element.SetVisible(false);
  EXPECT_EQ(0.0f, element.opacity());

  // Enable, and ensure that the element transiently disappears.
  element.SetVisible(true);
  EXPECT_FALSE(DoBeginFrame(&element, 0));
  EXPECT_FALSE(DoBeginFrame(&element, 10));
  EXPECT_EQ(element.opacity_when_visible(), element.opacity());
  EXPECT_TRUE(DoBeginFrame(&element, 2010));
  EXPECT_EQ(0.0f, element.opacity());

  EXPECT_FALSE(DoBeginFrame(&element, 2020));
  // Enable, and ensure that the element transiently disappears using
  // SetVisibleImmediately.
  element.SetVisibleImmediately(true);
  EXPECT_FALSE(DoBeginFrame(&element, 2020));
  EXPECT_EQ(element.opacity_when_visible(), element.opacity());
  EXPECT_TRUE(DoBeginFrame(&element, 4020));
  EXPECT_EQ(0.0f, element.opacity());

  EXPECT_FALSE(DoBeginFrame(&element, 4030));
  element.SetTransitionedProperties({OPACITY});
  element.SetVisible(true);
  EXPECT_TRUE(DoBeginFrame(&element, 4030));
  EXPECT_NE(element.opacity_when_visible(), element.opacity());
  element.SetVisibleImmediately(true);
  EXPECT_FALSE(DoBeginFrame(&element, 4030));
  EXPECT_EQ(element.opacity_when_visible(), element.opacity());
  EXPECT_TRUE(DoBeginFrame(&element, 6060));
  EXPECT_EQ(0.0f, element.GetTargetOpacity());
}

// Test that refreshing the visibility resets the transience timeout if the
// element is currently visible.
TEST(SimpleTransientElementTest, RefreshVisibility) {
  SimpleTransientElement element(base::TimeDelta::FromSeconds(2));
  element.SetOpacity(0.0f);

  // Enable, and ensure that the element is visible.
  element.SetVisible(true);
  EXPECT_EQ(element.opacity_when_visible(), element.opacity());
  EXPECT_FALSE(DoBeginFrame(&element, 0));
  EXPECT_FALSE(DoBeginFrame(&element, 1000));

  // Refresh visibility, and ensure that the element still transiently
  // disappears, but at a later time.
  element.RefreshVisible();
  EXPECT_FALSE(DoBeginFrame(&element, 1000));
  EXPECT_FALSE(DoBeginFrame(&element, 2000));
  EXPECT_EQ(element.opacity_when_visible(), element.opacity());
  EXPECT_TRUE(DoBeginFrame(&element, 3000));
  EXPECT_EQ(0.0f, element.opacity());

  // Refresh visibility, and ensure that disabling hides the element.
  element.SetVisible(true);
  EXPECT_FALSE(DoBeginFrame(&element, 3000));
  EXPECT_EQ(element.opacity_when_visible(), element.opacity());
  element.RefreshVisible();
  EXPECT_FALSE(DoBeginFrame(&element, 3000));
  EXPECT_FALSE(DoBeginFrame(&element, 4000));
  EXPECT_EQ(element.opacity_when_visible(), element.opacity());
  element.SetVisible(false);
  EXPECT_EQ(0.0f, element.opacity());
}

// Test that changing visibility on transient parent has the same effect on its
// children.
// TODO(mthiesse, vollick): Convert this test to use bindings.
TEST(SimpleTransientElementTest, VisibilityChildren) {
  UiScene scene;
  // Create transient root.
  auto transient_element =
      std::make_unique<SimpleTransientElement>(base::TimeDelta::FromSeconds(2));
  SimpleTransientElement* parent = transient_element.get();
  transient_element->set_opacity_when_visible(0.5);
  scene.AddUiElement(kRoot, std::move(transient_element));

  // Create child.
  auto element = std::make_unique<UiElement>();
  UiElement* child = element.get();
  element->set_opacity_when_visible(0.5);
  element->SetVisible(true);
  parent->AddChild(std::move(element));

  // Child hidden because parent is hidden.
  EXPECT_TRUE(scene.OnBeginFrame(MsToTicks(0), kStartHeadPose));
  EXPECT_FALSE(child->IsVisible());
  EXPECT_FALSE(parent->IsVisible());

  // Setting visiblity on parent should make the child visible.
  parent->SetVisible(true);
  scene.set_dirty();
  EXPECT_TRUE(scene.OnBeginFrame(MsToTicks(10), kStartHeadPose));
  EXPECT_TRUE(child->IsVisible());
  EXPECT_TRUE(parent->IsVisible());

  // Make sure the elements go away.
  EXPECT_TRUE(scene.OnBeginFrame(MsToTicks(2010), kStartHeadPose));
  EXPECT_FALSE(child->IsVisible());
  EXPECT_FALSE(parent->IsVisible());

  // Test again, but this time manually set the visibility to false.
  parent->SetVisible(true);
  scene.set_dirty();
  EXPECT_TRUE(scene.OnBeginFrame(MsToTicks(2020), kStartHeadPose));
  EXPECT_TRUE(child->IsVisible());
  EXPECT_TRUE(parent->IsVisible());
  parent->SetVisible(false);
  scene.set_dirty();
  EXPECT_TRUE(scene.OnBeginFrame(MsToTicks(2030), kStartHeadPose));
  EXPECT_FALSE(child->IsVisible());
  EXPECT_FALSE(parent->IsVisible());
}

class ShowUntilSignalElementTest : public testing::Test {
 public:
  ShowUntilSignalElementTest() {}

  void SetUp() override {
    ResetCallbackTriggered();
    element_ = std::make_unique<ShowUntilSignalTransientElement>(
        base::TimeDelta::FromSeconds(2), base::TimeDelta::FromSeconds(5),
        base::BindRepeating(&ShowUntilSignalElementTest::OnMinDuration,
                            base::Unretained(this)),
        base::BindRepeating(&ShowUntilSignalElementTest::OnTimeout,
                            base::Unretained(this)));
  }

  ShowUntilSignalTransientElement& element() { return *element_; }
  TransientElementHideReason hide_reason() { return hide_reason_; }
  bool min_duration_callback_triggered() {
    return min_duration_callback_triggered_;
  }
  bool hide_callback_triggered() { return hide_callback_triggered_; }

  void OnMinDuration() { min_duration_callback_triggered_ = true; }

  void OnTimeout(TransientElementHideReason reason) {
    hide_callback_triggered_ = true;
    hide_reason_ = reason;
  }

  void ResetCallbackTriggered() {
    min_duration_callback_triggered_ = false;
    hide_callback_triggered_ = false;
  }

  void VerifyElementHidesAfterSignal() {
    EXPECT_FALSE(element().IsVisible());

    // Make element visible.
    element().SetVisible(true);
    EXPECT_FALSE(DoBeginFrame(&element(), 10));
    EXPECT_EQ(element().opacity_when_visible(), element().opacity());

    // Signal, element should still be visible since time < min duration.
    element().Signal(true);
    EXPECT_FALSE(DoBeginFrame(&element(), 200));
    EXPECT_EQ(element().opacity_when_visible(), element().opacity());

    // Element hides and callback triggered.
    EXPECT_TRUE(DoBeginFrame(&element(), 2010));
    EXPECT_EQ(0.0f, element().opacity());
    EXPECT_TRUE(min_duration_callback_triggered());
    EXPECT_TRUE(hide_callback_triggered());
    ResetCallbackTriggered();
    EXPECT_EQ(TransientElementHideReason::kSignal, hide_reason());
  }

 private:
  bool min_duration_callback_triggered_ = false;
  bool hide_callback_triggered_ = false;
  TransientElementHideReason hide_reason_;
  std::unique_ptr<ShowUntilSignalTransientElement> element_;
};

// Test that the element disappears when signalled.
TEST_F(ShowUntilSignalElementTest, ElementHidesAfterSignal) {
  // We run this twice to verify that an element can be shown again after being
  // hidden.
  VerifyElementHidesAfterSignal();
  VerifyElementHidesAfterSignal();
}

// Test that the transient element times out.
TEST_F(ShowUntilSignalElementTest, TimedOut) {
  EXPECT_FALSE(element().IsVisible());

  // Make element visible.
  element().SetVisible(true);
  EXPECT_FALSE(DoBeginFrame(&element(), 10));
  EXPECT_EQ(element().opacity_when_visible(), element().opacity());

  // Element should be visible since we haven't signalled.
  EXPECT_FALSE(DoBeginFrame(&element(), 2010));
  EXPECT_TRUE(min_duration_callback_triggered());
  EXPECT_FALSE(hide_callback_triggered());
  ResetCallbackTriggered();
  EXPECT_EQ(element().opacity_when_visible(), element().opacity());

  // Element hides and callback triggered.
  EXPECT_TRUE(DoBeginFrame(&element(), 6010));
  EXPECT_EQ(0.0f, element().opacity());
  EXPECT_FALSE(min_duration_callback_triggered());
  EXPECT_TRUE(hide_callback_triggered());
  EXPECT_EQ(TransientElementHideReason::kTimeout, hide_reason());
}

// Test that refreshing the visibility resets the transience timeout if the
// element is currently visible.
TEST_F(ShowUntilSignalElementTest, RefreshVisibility) {
  // Enable, and ensure that the element is visible.
  element().SetVisible(true);
  EXPECT_EQ(element().opacity_when_visible(), element().opacity());
  EXPECT_FALSE(DoBeginFrame(&element(), 0));
  EXPECT_FALSE(DoBeginFrame(&element(), 1000));
  element().Signal(true);

  // Refresh visibility, and ensure that the element still transiently
  // disappears, but at a later time.
  element().RefreshVisible();
  EXPECT_FALSE(DoBeginFrame(&element(), 1000));
  EXPECT_FALSE(DoBeginFrame(&element(), 2500));
  EXPECT_EQ(element().opacity_when_visible(), element().opacity());
  EXPECT_TRUE(DoBeginFrame(&element(), 3000));
  EXPECT_EQ(0.0f, element().opacity());
  EXPECT_EQ(TransientElementHideReason::kSignal, hide_reason());
}

}  // namespace vr
