// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/location_bar/page_info_bubble_decoration.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "components/toolbar/test_toolbar_model.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class LocationBarViewMacToolbarModel : public TestToolbarModel {
  base::string16 GetSecureVerboseText() const override {
    if (IsOfflinePage())
      return base::ASCIIToUTF16("test");

    switch (GetSecurityLevel(false)) {
      case security_state::HTTP_SHOW_WARNING:
      case security_state::EV_SECURE:
      case security_state::SECURE:
      case security_state::DANGEROUS:
        return base::ASCIIToUTF16("test");
      default:
        return base::string16();
    }
  }

  base::string16 GetEVCertName() const override {
    return base::ASCIIToUTF16("test cert");
  }
};

// Mocks the OmniboxView so that we can set if the omnibox is empty or not.
class MockOmniboxView : public OmniboxViewMac {
 public:
  MockOmniboxView(OmniboxEditController* controller,
                  Profile* profile,
                  CommandUpdater* command_updater,
                  AutocompleteTextField* field)
      : OmniboxViewMac(controller, profile, command_updater, field) {}

  // Sets if the MockOmniboxView should be empty or not.
  void set_is_empty(bool is_empty) { is_empty_ = is_empty; }

  // Returns 0 if |is_empty_| is false. Otherwise, return 1.
  int GetOmniboxTextLength() const override { return is_empty_ ? 0 : 1; }

 private:
  // True if the OmniboxView should be empty.
  bool is_empty_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockOmniboxView);
};

// Testing class for LocationBarViewMac. This class is used mock the
// ToolbarModel.
class TestingLocationBarViewMac : public LocationBarViewMac {
 public:
  TestingLocationBarViewMac(AutocompleteTextField* field,
                            CommandUpdater* command_updater,
                            Profile* profile,
                            Browser* browser)
      : LocationBarViewMac(field, command_updater, profile, browser) {}

  // Overridden so that LocationBarViewMac will use the test ToolbarModel
  // instead.
  const ToolbarModel* GetToolbarModel() const override {
    return &toolbar_model_;
  }

  // Overridden so that LocationBarViewMac will use the test ToolbarModel
  // instead.
  ToolbarModel* GetToolbarModel() override { return &toolbar_model_; }

  // Sets the security level of |toolbar_model_|.
  void SetSecurityLevel(security_state::SecurityLevel level) {
    toolbar_model_.set_security_level(level);
  }

 private:
  // The toolbar model used for testing.
  LocationBarViewMacToolbarModel toolbar_model_;

  DISALLOW_COPY_AND_ASSIGN(TestingLocationBarViewMac);
};

// Testing class for TestingPageInfoBubbleDecoration.
class TestingPageInfoBubbleDecoration : public PageInfoBubbleDecoration {
 public:
  explicit TestingPageInfoBubbleDecoration(LocationBarViewMac* owner)
      : PageInfoBubbleDecoration(owner) {}

  void AnimateIn(bool image_fade = true) override {
    has_animated_ = true;
    is_showing_ = true;
    PageInfoBubbleDecoration::AnimateIn(image_fade);
  }

  void AnimateOut() override {
    has_animated_ = true;
    is_showing_ = false;
    PageInfoBubbleDecoration::AnimateOut();
  }

  void ShowWithoutAnimation() override {
    has_animated_ = false;
    is_showing_ = true;
    PageInfoBubbleDecoration::ShowWithoutAnimation();
  }

  bool HasAnimatedOut() const override { return !is_showing_; }

  bool AnimatingOut() const override { return false; }

  void ResetAnimation() override {
    has_animated_ = false;
    PageInfoBubbleDecoration::ResetAnimation();
  }

  bool has_animated() const { return has_animated_; };

  void ResetAnimationFlag() { has_animated_ = false; }

  void ResetIsShowing() { is_showing_ = false; }

 private:
  // True if the decoration has animated.
  bool has_animated_ = false;

  // True if the decoration is showing.
  bool is_showing_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestingPageInfoBubbleDecoration);
};

class LocationBarViewMacTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    // Width must be large, otherwise it'll be too narrow to fit the security
    // state decoration.
    NSRect frame = NSMakeRect(0, 0, 500, 30);
    field_.reset([[AutocompleteTextField alloc] initWithFrame:frame]);

    location_bar_.reset(new TestingLocationBarViewMac(
        field_.get(), browser()->command_controller(), browser()->profile(),
        browser()));

    location_bar_->page_info_decoration_.reset(
        new TestingPageInfoBubbleDecoration(location_bar_.get()));
    decoration()->disable_animations_during_testing_ = true;

    omnibox_view_ =
        new MockOmniboxView(nullptr, browser()->profile(),
                            browser()->command_controller(), field_.get());

    location_bar_->omnibox_view_.reset(omnibox_view_);
  }

  void TearDown() override {
    location_bar_.reset();
    CocoaProfileTest::TearDown();
  }

  TestingPageInfoBubbleDecoration* decoration() const {
    TestingPageInfoBubbleDecoration* decoration =
        static_cast<TestingPageInfoBubbleDecoration*>(
            location_bar_->page_info_decoration_.get());

    return decoration;
  }

  bool IsDecorationLabelEmpty() const {
    return !decoration()->full_label_ || ![decoration()->full_label_ length];
  }

  TestingLocationBarViewMac* location_bar() const {
    return location_bar_.get();
  }

  MockOmniboxView* omnibox_view() const {
    MockOmniboxView* omnibox_view =
        static_cast<MockOmniboxView*>(location_bar_->omnibox_view_.get());
    return omnibox_view;
  }

  AutocompleteTextField* field() const { return field_.get(); }

  void AnimatePageInfoIfPossible(bool tab_changed) {
    location_bar_->Layout();
    location_bar_->AnimatePageInfoIfPossible(tab_changed);
  }

 protected:
  LocationBarViewMacTest() {}

 private:
  // The LocationBarView object we're testing.
  std::unique_ptr<TestingLocationBarViewMac> location_bar_;

  // The autocomplete text field.
  base::scoped_nsobject<AutocompleteTextField> field_;

  // The mocked omnibox view. Weak, owned by |location_bar_|.
  MockOmniboxView* omnibox_view_;

  DISALLOW_COPY_AND_ASSIGN(LocationBarViewMacTest);
};

// Tests the security decoration's visibility and animation without any tab or
// width changes.
TEST_F(LocationBarViewMacTest, ShowAndAnimatePageInfoDecoration) {
  // Set the security level to DANGEROUS. The decoration should animate in.
  location_bar()->SetSecurityLevel(security_state::DANGEROUS);
  AnimatePageInfoIfPossible(false);
  EXPECT_TRUE(decoration()->has_animated());
  EXPECT_FALSE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Set the security level to NONE. The decoration should animate out.
  location_bar()->SetSecurityLevel(security_state::NONE);
  AnimatePageInfoIfPossible(false);
  EXPECT_FALSE(decoration()->has_animated());
  EXPECT_TRUE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Set the security level to SECURE. The decoration should show, but not
  // animate.
  location_bar()->SetSecurityLevel(security_state::SECURE);
  AnimatePageInfoIfPossible(false);
  EXPECT_FALSE(decoration()->has_animated());
  EXPECT_FALSE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Set the security level to EV_SECURE. The decoration should still show,
  // but not animate.
  location_bar()->SetSecurityLevel(security_state::EV_SECURE);
  AnimatePageInfoIfPossible(false);
  EXPECT_FALSE(decoration()->has_animated());
  EXPECT_FALSE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Set the security level to NONE. The decoration should still hide without
  // animating out.
  location_bar()->SetSecurityLevel(security_state::NONE);
  AnimatePageInfoIfPossible(false);
  EXPECT_FALSE(decoration()->has_animated());
  EXPECT_TRUE(IsDecorationLabelEmpty());
}

// Tests the security decoration's visibility and animation when the omnibox
// is updated from a switched tab.
TEST_F(LocationBarViewMacTest, PageInfoDecorationWithTabChanges) {
  // Show nonsecure decoration.
  location_bar()->SetSecurityLevel(security_state::HTTP_SHOW_WARNING);
  AnimatePageInfoIfPossible(false);
  EXPECT_TRUE(decoration()->has_animated());

  decoration()->ResetAnimationFlag();

  // Switch to a tab with no decoration.
  location_bar()->SetSecurityLevel(security_state::NONE);
  AnimatePageInfoIfPossible(true);
  EXPECT_FALSE(decoration()->has_animated());
  EXPECT_TRUE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Switch back to the tab with the nonsecure decoration.
  location_bar()->SetSecurityLevel(security_state::HTTP_SHOW_WARNING);
  AnimatePageInfoIfPossible(true);
  EXPECT_FALSE(decoration()->has_animated());

  decoration()->ResetAnimationFlag();

  // Show the secure decoration.
  location_bar()->SetSecurityLevel(security_state::SECURE);
  AnimatePageInfoIfPossible(false);
  EXPECT_FALSE(decoration()->has_animated());

  decoration()->ResetAnimationFlag();

  // Switch to a tab with no decoration.
  location_bar()->SetSecurityLevel(security_state::NONE);
  AnimatePageInfoIfPossible(true);
  EXPECT_FALSE(decoration()->has_animated());
  EXPECT_TRUE(IsDecorationLabelEmpty());
}

// Tests the security decoration's visibility and animation when the omnibox
// is empty.
TEST_F(LocationBarViewMacTest, PageInfoDecorationWithEmptyOmnibox) {
  // Set the omnibox to empty and then set the security level to nonsecure.
  // The decoration should not appear.
  omnibox_view()->set_is_empty(true);
  location_bar()->SetSecurityLevel(security_state::DANGEROUS);
  AnimatePageInfoIfPossible(false);
  EXPECT_FALSE(decoration()->has_animated());
  EXPECT_TRUE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Set the omnibox to nonempty. The decoration should now appear.
  omnibox_view()->set_is_empty(false);
  decoration()->ResetIsShowing();
  AnimatePageInfoIfPossible(false);
  EXPECT_TRUE(decoration()->has_animated());
  EXPECT_FALSE(IsDecorationLabelEmpty());
}

// Tests to see that the security decoration animates out when the omnibox's
// width becomes narrow.
TEST_F(LocationBarViewMacTest, PageInfoDecorationWidthChanges) {
  // Show the nonsecure decoration.
  location_bar()->SetSecurityLevel(security_state::HTTP_SHOW_WARNING);
  AnimatePageInfoIfPossible(false);
  EXPECT_TRUE(decoration()->has_animated());
  EXPECT_FALSE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Make the omnibox narrow.
  [field() setFrame:NSMakeRect(0, 0, 119, 30)];
  AnimatePageInfoIfPossible(false);
  EXPECT_TRUE(decoration()->has_animated());
  EXPECT_TRUE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Make the omnibox wide again.
  [field() setFrame:NSMakeRect(0, 0, 500, 30)];
  AnimatePageInfoIfPossible(false);
  EXPECT_TRUE(decoration()->has_animated());
  EXPECT_FALSE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Show secure decoration.
  location_bar()->SetSecurityLevel(security_state::SECURE);
  AnimatePageInfoIfPossible(false);
  EXPECT_FALSE(decoration()->has_animated());
  EXPECT_FALSE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Make the omnibox narrow.
  [field() setFrame:NSMakeRect(0, 0, 119, 30)];
  AnimatePageInfoIfPossible(false);
  EXPECT_TRUE(decoration()->has_animated());
  EXPECT_TRUE(IsDecorationLabelEmpty());

  decoration()->ResetAnimationFlag();

  // Make the omnibox wide again.
  [field() setFrame:NSMakeRect(0, 0, 600, 30)];
  AnimatePageInfoIfPossible(false);
  EXPECT_TRUE(decoration()->has_animated());
  EXPECT_FALSE(IsDecorationLabelEmpty());
}

}  // namespace
