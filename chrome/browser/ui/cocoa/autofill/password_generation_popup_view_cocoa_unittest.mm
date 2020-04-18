// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/password_generation_popup_view_cocoa.h"

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "components/autofill/core/browser/suggestion.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/range/range.h"

using testing::AtLeast;
using testing::Return;

namespace {

class MockPasswordGenerationPopupController
   : public autofill::PasswordGenerationPopupController {
 public:
  MockPasswordGenerationPopupController()
    : help_text_(base::ASCIIToUTF16("Help me if you can I'm feeling dooown")),
      popup_bounds_(gfx::Rect(0, 0, 200, 100)) {}

  MOCK_METHOD0(PasswordAccepted, void());

  void OnSavedPasswordsLinkClicked() override {}

  int GetMinimumWidth() override { return 200; }

  bool display_password() const override { return true; }

  bool password_selected() const override { return false; }

  MOCK_CONST_METHOD0(password, base::string16());

  base::string16 SuggestedText() override {
    return base::ASCIIToUTF16("Suggested by Chrome");
  }

  const base::string16& HelpText() override { return help_text_; }

  const gfx::Range& HelpTextLinkRange() override { return link_range_; }

  // AutofillPopupViewDelegate implementation.
  void Hide() override {}
  MOCK_METHOD0(ViewDestroyed, void());
  void SetSelectionAtPoint(const gfx::Point&) override {}
  bool AcceptSelectedLine() override { return true; }
  void SelectionCleared() override {}
  bool HasSelection() const override { return password_selected(); }
  gfx::Rect popup_bounds() const override { return popup_bounds_; }
  MOCK_METHOD0(container_view, gfx::NativeView());
  MOCK_CONST_METHOD0(element_bounds, gfx::RectF&());
  MOCK_CONST_METHOD0(IsRTL, bool());
  MOCK_METHOD0(GetSuggestions, const std::vector<autofill::Suggestion>());
  void SetTypesetter(gfx::Typesetter Typesetter) override {}
  MOCK_METHOD1(GetElidedValueWidthForRow, int(int));
  MOCK_METHOD1(GetElidedLabelWidthForRow, int(int));

 private:
  base::string16 help_text_;
  gfx::Range link_range_;

  const gfx::Rect popup_bounds_;

  DISALLOW_COPY_AND_ASSIGN(MockPasswordGenerationPopupController);
};

class PasswordGenerationPopupViewCocoaTest : public CocoaTest {
 protected:
  PasswordGenerationPopupViewCocoaTest()
    : password_(base::ASCIIToUTF16("wow! such password"))
  {}

  void SetUp() override {
    mock_controller_.reset(new MockPasswordGenerationPopupController);
    EXPECT_CALL(*mock_controller_, password())
        .WillRepeatedly(Return(password_));

    view_.reset([[PasswordGenerationPopupViewCocoa alloc]
        initWithController:mock_controller_.get()
                     frame:NSZeroRect]);

    NSView* contentView = [test_window() contentView];
    [contentView addSubview:view_];
    EXPECT_CALL(*mock_controller_, container_view())
        .WillRepeatedly(Return(contentView));
  }

  base::string16 password_;
  std::unique_ptr<MockPasswordGenerationPopupController> mock_controller_;
  base::scoped_nsobject<PasswordGenerationPopupViewCocoa> view_;
};

TEST_VIEW(PasswordGenerationPopupViewCocoaTest, view_);

TEST_F(PasswordGenerationPopupViewCocoaTest, ShowAndHide) {
  // Verify that the view fetches a password from the controller.
  EXPECT_CALL(*mock_controller_, password()).Times(AtLeast(1))
      .WillRepeatedly(Return(password_));

  view_.reset([[PasswordGenerationPopupViewCocoa alloc]
      initWithController:mock_controller_.get()
                   frame:NSZeroRect]);

  [view_ showPopup];
  [view_ display];
  [view_ hidePopup];
}

// Verifies that it doesn't crash when the controller is destroyed before the
// popup is hidden.
TEST_F(PasswordGenerationPopupViewCocoaTest, ControllerDestroyed) {
  [view_ showPopup];
  mock_controller_.reset();
  [view_ controllerDestroyed];
  [view_ display];
  [view_ hidePopup];
}

}  // namespace
