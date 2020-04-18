// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/command_updater_delegate.h"
#include "chrome/browser/command_updater_impl.h"
#include "chrome/browser/ui/cocoa/location_bar/manage_passwords_decoration.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/password_manager/core/common/password_manager_ui.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

// A simple CommandUpdaterDelegate for testing whether the correct command
// was sent.
class TestCommandUpdaterDelegate : public CommandUpdaterDelegate {
 public:
  TestCommandUpdaterDelegate() : id_(0) {}

  void ExecuteCommandWithDisposition(int id, WindowOpenDisposition disposition)
      override {
    id_ = id;
  }

  int id() { return id_; }

 private:
  int id_;
};

bool ImagesEqual(NSImage* left, NSImage* right) {
  if (!left || !right)
    return left == right;
  gfx::Image leftImage([left copy]);
  gfx::Image rightImage([right copy]);
  return leftImage.As1xPNGBytes()->Equals(rightImage.As1xPNGBytes());
}

}  // namespace

// Tests isolated functionality of the ManagedPasswordsDecoration.
class ManagePasswordsDecorationTest : public CocoaTest {
 public:
  ManagePasswordsDecorationTest()
      : commandUpdater_(&commandDelegate_),
        decoration_(&commandUpdater_, NULL) {
    commandUpdater_.UpdateCommandEnabled(IDC_MANAGE_PASSWORDS_FOR_PAGE, true);
  }

 protected:
  TestCommandUpdaterDelegate* commandDelegate() { return &commandDelegate_; }
  ManagePasswordsDecoration* decoration() { return &decoration_; }

 private:
  TestCommandUpdaterDelegate commandDelegate_;
  CommandUpdaterImpl commandUpdater_;
  ManagePasswordsDecoration decoration_;
};

TEST_F(ManagePasswordsDecorationTest, ExecutesManagePasswordsCommandOnClick) {
  EXPECT_EQ(AcceptsPress::ALWAYS, decoration()->AcceptsMousePress());
  EXPECT_TRUE(decoration()->OnMousePressed(NSRect(), NSPoint()));
  EXPECT_EQ(IDC_MANAGE_PASSWORDS_FOR_PAGE, commandDelegate()->id());
}

// Parameter object for ManagePasswordsDecorationStateTests.
struct ManagePasswordsTestCase {
  // Inputs
  password_manager::ui::State state;

  // Outputs
  bool visible;
  int toolTip;
};

// Tests that setting different combinations of password_manager::ui::State
// and the Active property of the decoration result in the correct visibility,
// decoration icon, and tooltip.
class ManagePasswordsDecorationStateTest
    : public ManagePasswordsDecorationTest,
      public ::testing::WithParamInterface<ManagePasswordsTestCase> {};

TEST_P(ManagePasswordsDecorationStateTest, TestState) {
  decoration()->icon()->SetState(GetParam().state);
  EXPECT_EQ(GetParam().visible, decoration()->IsVisible());
  const int kIconSize = 16;
  NSImage* expected_image =
      GetParam().state == password_manager::ui::INACTIVE_STATE
          ? nil
          : NSImageFromImageSkia(gfx::CreateVectorIcon(kKeyIcon, kIconSize,
                                                       gfx::kChromeIconGrey));
  EXPECT_TRUE(ImagesEqual(expected_image, decoration()->GetImage()));
  EXPECT_NSEQ(GetParam().toolTip
                  ? l10n_util::GetNSStringWithFixup(GetParam().toolTip)
                  : nil,
              decoration()->GetToolTip());
}

ManagePasswordsTestCase testCases[] = {
    {.state = password_manager::ui::INACTIVE_STATE,
     .visible = false,
     .toolTip = 0},
    {.state = password_manager::ui::MANAGE_STATE,
     .visible = true,
     .toolTip = IDS_PASSWORD_MANAGER_TOOLTIP_MANAGE},
    {.state = password_manager::ui::PENDING_PASSWORD_STATE,
     .visible = true,
     .toolTip = IDS_PASSWORD_MANAGER_TOOLTIP_SAVE}};

INSTANTIATE_TEST_CASE_P(,
                        ManagePasswordsDecorationStateTest,
                        ::testing::ValuesIn(testCases));
