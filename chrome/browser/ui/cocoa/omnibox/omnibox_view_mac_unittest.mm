// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/test/scoped_force_rtl_mac.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_edit_controller.h"
#include "chrome/browser/ui/toolbar/chrome_toolbar_model_delegate.h"
#include "chrome/test/base/testing_profile.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/omnibox/browser/omnibox_popup_view.h"
#include "components/toolbar/toolbar_model_impl.h"
#include "content/public/common/content_constants.h"
#include "testing/platform_test.h"
#include "ui/gfx/font.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"

namespace {

class MockOmniboxEditModel : public OmniboxEditModel {
 public:
  MockOmniboxEditModel(OmniboxView* view,
                       OmniboxEditController* controller,
                       Profile* profile)
      : OmniboxEditModel(
            view,
            controller,
            base::WrapUnique(new ChromeOmniboxClient(controller, profile))),
        up_or_down_count_(0) {}

  void OnUpOrDownKeyPressed(int count) override { up_or_down_count_ = count; }

  int up_or_down_count() const { return up_or_down_count_; }

  void set_up_or_down_count(int count) {
    up_or_down_count_ = count;
  }

 private:
  int up_or_down_count_;

  DISALLOW_COPY_AND_ASSIGN(MockOmniboxEditModel);
};

class MockOmniboxPopupView : public OmniboxPopupView {
 public:
  MockOmniboxPopupView() : is_open_(false) {}
  ~MockOmniboxPopupView() override {}

  // Overridden from OmniboxPopupView:
  bool IsOpen() const override { return is_open_; }
  void InvalidateLine(size_t line) override {}
  void OnLineSelected(size_t line) override {}
  void UpdatePopupAppearance() override {}
  void OnMatchIconUpdated(size_t match_index) override {}
  void PaintUpdatesNow() override {}
  void OnDragCanceled() override {}

  void set_is_open(bool is_open) { is_open_ = is_open; }

 private:
  bool is_open_;

  DISALLOW_COPY_AND_ASSIGN(MockOmniboxPopupView);
};

class TestingToolbarModelDelegate : public ChromeToolbarModelDelegate {
 public:
  TestingToolbarModelDelegate() {}
  ~TestingToolbarModelDelegate() override {}

  // Overridden from ChromeToolbarModelDelegate:
  content::WebContents* GetActiveWebContents() const override { return NULL; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestingToolbarModelDelegate);
};

class TestingOmniboxEditController : public ChromeOmniboxEditController {
 public:
  explicit TestingOmniboxEditController(ToolbarModel* toolbar_model)
      : ChromeOmniboxEditController(NULL), toolbar_model_(toolbar_model) {}
  ~TestingOmniboxEditController() override {}

 protected:
  // Overridden from ChromeOmniboxEditController:
  void UpdateWithoutTabRestore() override {}
  void OnChanged() override {}
  ToolbarModel* GetToolbarModel() override { return toolbar_model_; }
  const ToolbarModel* GetToolbarModel() const override {
    return toolbar_model_;
  }
  content::WebContents* GetWebContents() override { return nullptr; }

 private:
  ToolbarModel* toolbar_model_;

  DISALLOW_COPY_AND_ASSIGN(TestingOmniboxEditController);
};

}  // namespace

class OmniboxViewMacTest : public CocoaProfileTest {
 public:
  void SetModel(OmniboxViewMac* view, OmniboxEditModel* model) {
    view->model_.reset(model);
  }
};

TEST_F(OmniboxViewMacTest, GetFonts) {
  EXPECT_TRUE(OmniboxViewMac::GetNormalFieldFont());
  EXPECT_TRUE(OmniboxViewMac::GetBoldFieldFont());
  EXPECT_TRUE(OmniboxViewMac::GetLargeFont());
  EXPECT_TRUE(OmniboxViewMac::GetSmallFont());
}

TEST_F(OmniboxViewMacTest, TabToAutocomplete) {
  OmniboxViewMac view(NULL, profile(), NULL, NULL);

  // This is deleted by the omnibox view.
  MockOmniboxEditModel* model =
      new MockOmniboxEditModel(&view, NULL, profile());
  SetModel(&view, model);

  MockOmniboxPopupView popup_view;
  OmniboxPopupModel popup_model(&popup_view, model);

  // With popup closed verify that tab doesn't autocomplete.
  popup_view.set_is_open(false);
  view.OnDoCommandBySelector(@selector(insertTab:));
  EXPECT_EQ(0, model->up_or_down_count());
  view.OnDoCommandBySelector(@selector(insertBacktab:));
  EXPECT_EQ(0, model->up_or_down_count());

  // With popup open verify that tab does autocomplete.
  popup_view.set_is_open(true);
  view.OnDoCommandBySelector(@selector(insertTab:));
  EXPECT_EQ(1, model->up_or_down_count());
  view.OnDoCommandBySelector(@selector(insertBacktab:));
  EXPECT_EQ(-1, model->up_or_down_count());
}

TEST_F(OmniboxViewMacTest, UpDownArrow) {
  OmniboxViewMac view(NULL, profile(), NULL, NULL);

  // This is deleted by the omnibox view.
  MockOmniboxEditModel* model =
      new MockOmniboxEditModel(&view, NULL, profile());
  SetModel(&view, model);

  MockOmniboxPopupView popup_view;
  OmniboxPopupModel popup_model(&popup_view, model);

  // With popup closed verify that pressing up and down arrow works.
  popup_view.set_is_open(false);
  model->set_up_or_down_count(0);
  view.OnDoCommandBySelector(@selector(moveDown:));
  EXPECT_EQ(1, model->up_or_down_count());
  model->set_up_or_down_count(0);
  view.OnDoCommandBySelector(@selector(moveUp:));
  EXPECT_EQ(-1, model->up_or_down_count());

  // With popup open verify that pressing up and down arrow works.
  popup_view.set_is_open(true);
  model->set_up_or_down_count(0);
  view.OnDoCommandBySelector(@selector(moveDown:));
  EXPECT_EQ(1, model->up_or_down_count());
  model->set_up_or_down_count(0);
  view.OnDoCommandBySelector(@selector(moveUp:));
  EXPECT_EQ(-1, model->up_or_down_count());
}

TEST_F(OmniboxViewMacTest, WritingDirectionLTR) {
  TestingToolbarModelDelegate delegate;
  ToolbarModelImpl toolbar_model(&delegate, 32 * 1024);
  TestingOmniboxEditController edit_controller(&toolbar_model);
  OmniboxViewMac view(&edit_controller, profile(), NULL, NULL);

  // This is deleted by the omnibox view.
  MockOmniboxEditModel* model =
      new MockOmniboxEditModel(&view, &edit_controller, profile());
  MockOmniboxPopupView popup_view;
  OmniboxPopupModel popup_model(&popup_view, model);

  model->OnSetFocus(true);
  SetModel(&view, model);
  view.SetUserText(base::ASCIIToUTF16("foo.com"));
  model->OnChanged();

  base::scoped_nsobject<NSMutableAttributedString> string(
      [[NSMutableAttributedString alloc] initWithString:@"foo.com"]);
  view.ApplyTextStyle(string);

  NSParagraphStyle* paragraphStyle =
      [string attribute:NSParagraphStyleAttributeName
                 atIndex:0
          effectiveRange:NULL];
  DCHECK(paragraphStyle);
  EXPECT_EQ(paragraphStyle.alignment, NSLeftTextAlignment);
  EXPECT_EQ(paragraphStyle.baseWritingDirection, NSWritingDirectionLeftToRight);
}

TEST_F(OmniboxViewMacTest, WritingDirectionRTL) {
  cocoa_l10n_util::ScopedForceRTLMac rtl;

  TestingToolbarModelDelegate delegate;
  ToolbarModelImpl toolbar_model(&delegate, 32 * 1024);
  TestingOmniboxEditController edit_controller(&toolbar_model);
  OmniboxViewMac view(&edit_controller, profile(), NULL, NULL);

  // This is deleted by the omnibox view.
  MockOmniboxEditModel* model =
      new MockOmniboxEditModel(&view, &edit_controller, profile());
  MockOmniboxPopupView popup_view;
  OmniboxPopupModel popup_model(&popup_view, model);

  model->OnSetFocus(true);
  SetModel(&view, model);
  view.SetUserText(base::ASCIIToUTF16("foo.com"));
  model->OnChanged();

  base::scoped_nsobject<NSMutableAttributedString> string(
      [[NSMutableAttributedString alloc] initWithString:@"foo.com"]);
  view.ApplyTextStyle(string);

  NSParagraphStyle* paragraphStyle =
      [string attribute:NSParagraphStyleAttributeName
                 atIndex:0
          effectiveRange:NULL];
  DCHECK(paragraphStyle);
  EXPECT_EQ(paragraphStyle.alignment, NSRightTextAlignment);
  EXPECT_EQ(paragraphStyle.baseWritingDirection, NSWritingDirectionLeftToRight);
}
