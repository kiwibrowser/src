// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/window/dialog_client_view.h"

#include <map>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/test/test_layout_provider.h"
#include "ui/views/test/test_views.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {

// Base class for tests. Also acts as the dialog delegate and contents view for
// TestDialogClientView.
class DialogClientViewTest : public test::WidgetTest,
                             public DialogDelegateView {
 public:
  DialogClientViewTest() {}

  // testing::Test:
  void SetUp() override {
    WidgetTest::SetUp();

    // Note: not using DialogDelegate::CreateDialogWidget(..), since that can
    // alter the frame type according to the platform.
    widget_ = new views::Widget;
    Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_WINDOW);
    params.delegate = this;
    widget_->Init(params);
    EXPECT_EQ(this, GetContentsView());
  }

  void TearDown() override {
    widget_->CloseNow();
    WidgetTest::TearDown();
  }

  // DialogDelegateView:
  gfx::Size CalculatePreferredSize() const override { return preferred_size_; }
  gfx::Size GetMinimumSize() const override { return min_size_; }
  gfx::Size GetMaximumSize() const override { return max_size_; }
  ClientView* CreateClientView(Widget* widget) override {
    client_view_ = new DialogClientView(widget, this);
    return client_view_;
  }

  bool ShouldUseCustomFrame() const override { return false; }

  void DeleteDelegate() override {
    // DialogDelegateView would delete this, but |this| is owned by the test.
  }

  View* CreateExtraView() override { return next_extra_view_.release(); }

  bool GetExtraViewPadding(int* padding) override {
    if (extra_view_padding_)
      *padding = *extra_view_padding_;
    return extra_view_padding_.get() != nullptr;
  }

  int GetDialogButtons() const override { return dialog_buttons_; }
  int GetDefaultDialogButton() const override {
    return default_button_.value_or(
        DialogDelegateView::GetDefaultDialogButton());
  }
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override {
    return button == ui::DIALOG_BUTTON_CANCEL && !cancel_label_.empty()
               ? cancel_label_
               : DialogDelegate::GetDialogButtonLabel(button);
  }

 protected:
  gfx::Rect GetUpdatedClientBounds() {
    client_view_->SizeToPreferredSize();
    client_view_->Layout();
    return client_view_->bounds();
  }

  // Makes sure that the content view is sized correctly. Width must be at least
  // the requested amount, but height should always match exactly.
  void CheckContentsIsSetToPreferredSize() {
    const gfx::Rect client_bounds = GetUpdatedClientBounds();
    const gfx::Size preferred_size = this->GetPreferredSize();
    EXPECT_EQ(preferred_size.height(), this->bounds().height());
    EXPECT_LE(preferred_size.width(), this->bounds().width());
    EXPECT_EQ(gfx::Point(), this->origin());
    EXPECT_EQ(client_bounds.width(), this->width());
  }

  // Sets the buttons to show in the dialog and refreshes the dialog.
  void SetDialogButtons(int dialog_buttons) {
    dialog_buttons_ = dialog_buttons;
    DialogModelChanged();
  }

  // Sets the view to provide to CreateExtraView() and updates the dialog. This
  // can only be called a single time because DialogClientView caches the result
  // of CreateExtraView() and never calls it again.
  void SetExtraView(View* view) {
    EXPECT_FALSE(next_extra_view_);
    next_extra_view_ = base::WrapUnique(view);
    DialogModelChanged();
    EXPECT_FALSE(next_extra_view_);
  }

  // Sets the extra view padding.
  void SetExtraViewPadding(int padding) {
    DCHECK(!extra_view_padding_);
    extra_view_padding_.reset(new int(padding));
    DialogModelChanged();
  }

  void SetSizeConstraints(const gfx::Size& min_size,
                          const gfx::Size& preferred_size,
                          const gfx::Size& max_size) {
    min_size_ = min_size;
    preferred_size_ = preferred_size;
    max_size_ = max_size;
  }

  View* FocusableViewAfter(View* view) {
    const bool dont_loop = false;
    const bool reverse = false;
    return GetFocusManager()->GetNextFocusableView(view, GetWidget(), reverse,
                                                   dont_loop);
  }

  // Set a longer than normal Cancel label so that the minimum button width is
  // exceeded. The resulting width is around 160 pixels, but depends on system
  // fonts.
  void SetLongCancelLabel() {
    cancel_label_ = base::ASCIIToUTF16("Cancel Cancel Cancel");
  }

  void set_default_button(int button) { default_button_ = button; }

  DialogClientView* client_view() { return client_view_; }

  Widget* widget() { return widget_; }

 private:
  // The dialog Widget.
  Widget* widget_ = nullptr;

  // The DialogClientView that's being tested. Owned by |widget_|.
  DialogClientView* client_view_;

  // The bitmask of buttons to show in the dialog.
  int dialog_buttons_ = ui::DIALOG_BUTTON_NONE;

  // Set and cleared in SetExtraView().
  std::unique_ptr<View> next_extra_view_;

  std::unique_ptr<int> extra_view_padding_;

  gfx::Size preferred_size_;
  gfx::Size min_size_;
  gfx::Size max_size_;

  base::string16 cancel_label_;  // If set, the label for the Cancel button.
  base::Optional<int> default_button_;

  DISALLOW_COPY_AND_ASSIGN(DialogClientViewTest);
};

TEST_F(DialogClientViewTest, UpdateButtons) {
  // This dialog should start with no buttons.
  EXPECT_EQ(GetDialogButtons(), ui::DIALOG_BUTTON_NONE);
  EXPECT_EQ(NULL, client_view()->ok_button());
  EXPECT_EQ(NULL, client_view()->cancel_button());
  const int height_without_buttons = GetUpdatedClientBounds().height();

  // Update to use both buttons.
  SetDialogButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  EXPECT_TRUE(client_view()->ok_button()->is_default());
  EXPECT_FALSE(client_view()->cancel_button()->is_default());
  const int height_with_buttons = GetUpdatedClientBounds().height();
  EXPECT_GT(height_with_buttons, height_without_buttons);

  // Remove the dialog buttons.
  SetDialogButtons(ui::DIALOG_BUTTON_NONE);
  EXPECT_EQ(NULL, client_view()->ok_button());
  EXPECT_EQ(NULL, client_view()->cancel_button());
  EXPECT_EQ(GetUpdatedClientBounds().height(), height_without_buttons);

  // Reset with just an ok button.
  SetDialogButtons(ui::DIALOG_BUTTON_OK);
  EXPECT_TRUE(client_view()->ok_button()->is_default());
  EXPECT_EQ(NULL, client_view()->cancel_button());
  EXPECT_EQ(GetUpdatedClientBounds().height(), height_with_buttons);

  // Reset with just a cancel button.
  SetDialogButtons(ui::DIALOG_BUTTON_CANCEL);
  EXPECT_EQ(NULL, client_view()->ok_button());
  EXPECT_EQ(client_view()->cancel_button()->is_default(),
            PlatformStyle::kDialogDefaultButtonCanBeCancel);
  EXPECT_EQ(GetUpdatedClientBounds().height(), height_with_buttons);
}

TEST_F(DialogClientViewTest, RemoveAndUpdateButtons) {
  // Removing buttons from another context should clear the local pointer.
  SetDialogButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  delete client_view()->ok_button();
  EXPECT_EQ(NULL, client_view()->ok_button());
  delete client_view()->cancel_button();
  EXPECT_EQ(NULL, client_view()->cancel_button());

  // Updating should restore the requested buttons properly.
  SetDialogButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  EXPECT_TRUE(client_view()->ok_button()->is_default());
  EXPECT_FALSE(client_view()->cancel_button()->is_default());
}

// Test that views inside the dialog client view have the correct focus order.
TEST_F(DialogClientViewTest, SetupFocusChain) {
#if defined(OS_WIN) || defined(OS_CHROMEOS)
  const bool kIsOkButtonOnLeftSide = true;
#else
  const bool kIsOkButtonOnLeftSide = false;
#endif

  GetContentsView()->SetFocusBehavior(View::FocusBehavior::ALWAYS);
  // Initially the dialog client view only contains the content view.
  EXPECT_EQ(GetContentsView(), FocusableViewAfter(GetContentsView()));

  // Add OK and cancel buttons.
  SetDialogButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);

  if (kIsOkButtonOnLeftSide) {
    EXPECT_EQ(client_view()->ok_button(),
              FocusableViewAfter(GetContentsView()));
    EXPECT_EQ(client_view()->cancel_button(),
              FocusableViewAfter(client_view()->ok_button()));
    EXPECT_EQ(GetContentsView(),
              FocusableViewAfter(client_view()->cancel_button()));
  } else {
    EXPECT_EQ(client_view()->cancel_button(),
              FocusableViewAfter(GetContentsView()));
    EXPECT_EQ(client_view()->ok_button(),
              FocusableViewAfter(client_view()->cancel_button()));
    EXPECT_EQ(GetContentsView(),
              FocusableViewAfter(client_view()->ok_button()));
  }

  // Add extra view and remove OK button.
  View* extra_view = new StaticSizedView(gfx::Size(200, 200));
  extra_view->SetFocusBehavior(View::FocusBehavior::ALWAYS);
  SetExtraView(extra_view);
  SetDialogButtons(ui::DIALOG_BUTTON_CANCEL);

  EXPECT_EQ(extra_view, FocusableViewAfter(GetContentsView()));
  EXPECT_EQ(client_view()->cancel_button(), FocusableViewAfter(extra_view));
  EXPECT_EQ(GetContentsView(), FocusableViewAfter(client_view()));

  // Add a dummy view to the contents view. Consult the FocusManager for the
  // traversal order since it now spans different levels of the view hierarchy.
  View* dummy_view = new StaticSizedView(gfx::Size(200, 200));
  dummy_view->SetFocusBehavior(View::FocusBehavior::ALWAYS);
  GetContentsView()->SetFocusBehavior(View::FocusBehavior::NEVER);
  GetContentsView()->AddChildView(dummy_view);
  EXPECT_EQ(dummy_view, FocusableViewAfter(client_view()->cancel_button()));
  EXPECT_EQ(extra_view, FocusableViewAfter(dummy_view));
  EXPECT_EQ(client_view()->cancel_button(), FocusableViewAfter(extra_view));

  // Views are added to the contents view, not the client view, so the focus
  // chain within the client view is not affected.
  EXPECT_EQ(nullptr, client_view()->cancel_button()->GetNextFocusableView());
}

// Test that the contents view gets its preferred size in the basic dialog
// configuration.
TEST_F(DialogClientViewTest, ContentsSize) {
  CheckContentsIsSetToPreferredSize();
  EXPECT_EQ(GetContentsView()->size(), client_view()->size());
  // There's nothing in the contents view (i.e. |this|), so it should be 0x0.
  EXPECT_EQ(gfx::Size(), client_view()->size());
}

// Test the effect of the button strip on layout.
TEST_F(DialogClientViewTest, LayoutWithButtons) {
  SetDialogButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  CheckContentsIsSetToPreferredSize();

  EXPECT_LT(GetContentsView()->bounds().bottom(),
            client_view()->bounds().bottom());
  const gfx::Size no_extra_view_size = client_view()->bounds().size();

  View* extra_view = new StaticSizedView(gfx::Size(200, 200));
  SetExtraView(extra_view);
  CheckContentsIsSetToPreferredSize();
  EXPECT_GT(client_view()->bounds().height(), no_extra_view_size.height());
  const int width_of_dialog_small_padding = client_view()->width();

  // Try with an adjusted padding for the extra view.
  SetExtraViewPadding(250);
  CheckContentsIsSetToPreferredSize();
  EXPECT_GT(client_view()->bounds().width(), width_of_dialog_small_padding);

  const gfx::Size with_extra_view_size = client_view()->size();
  EXPECT_NE(no_extra_view_size, with_extra_view_size);

  // Hiding the extra view removes it as well as the extra padding.
  extra_view->SetVisible(false);
  CheckContentsIsSetToPreferredSize();
  EXPECT_EQ(no_extra_view_size, client_view()->size());

  // Making it visible again adds it all back.
  extra_view->SetVisible(true);
  CheckContentsIsSetToPreferredSize();
  EXPECT_EQ(with_extra_view_size, client_view()->size());

  // Leave |extra_view| hidden. It should still have a parent, to ensure it is
  // owned by a View hierarchy and gets deleted.
  extra_view->SetVisible(false);
  EXPECT_TRUE(extra_view->parent());
}

// Ensure the minimum, maximum and preferred sizes of the contents view are
// respected by the client view, and that the client view includes the button
// row in its minimum and preferred size calculations.
TEST_F(DialogClientViewTest, MinMaxPreferredSize) {
  SetDialogButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  const gfx::Size buttons_size = client_view()->GetPreferredSize();
  EXPECT_FALSE(buttons_size.IsEmpty());

  // When the contents view has no preference, just fit the buttons. The
  // maximum size should be unconstrained in both directions.
  EXPECT_EQ(buttons_size, client_view()->GetMinimumSize());
  EXPECT_EQ(gfx::Size(), client_view()->GetMaximumSize());

  // Ensure buttons are between these widths, for the constants below.
  EXPECT_LT(20, buttons_size.width());
  EXPECT_GT(300, buttons_size.width());

  // With no buttons, client view should match the contents view.
  SetDialogButtons(ui::DIALOG_BUTTON_NONE);
  SetSizeConstraints(gfx::Size(10, 15), gfx::Size(20, 25), gfx::Size(300, 350));
  EXPECT_EQ(gfx::Size(10, 15), client_view()->GetMinimumSize());
  EXPECT_EQ(gfx::Size(20, 25), client_view()->GetPreferredSize());
  EXPECT_EQ(gfx::Size(300, 350), client_view()->GetMaximumSize());

  // With buttons, size should increase vertically only.
  SetDialogButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  EXPECT_EQ(gfx::Size(buttons_size.width(), 15 + buttons_size.height()),
            client_view()->GetMinimumSize());
  EXPECT_EQ(gfx::Size(buttons_size.width(), 25 + buttons_size.height()),
            client_view()->GetPreferredSize());
  EXPECT_EQ(gfx::Size(300, 350 + buttons_size.height()),
            client_view()->GetMaximumSize());

  // If the contents view gets bigger, it should take over the width.
  SetSizeConstraints(gfx::Size(400, 450), gfx::Size(500, 550),
                     gfx::Size(600, 650));
  EXPECT_EQ(gfx::Size(400, 450 + buttons_size.height()),
            client_view()->GetMinimumSize());
  EXPECT_EQ(gfx::Size(500, 550 + buttons_size.height()),
            client_view()->GetPreferredSize());
  EXPECT_EQ(gfx::Size(600, 650 + buttons_size.height()),
            client_view()->GetMaximumSize());
}

// Ensure button widths are linked under MD.
TEST_F(DialogClientViewTest, LinkedWidths) {
  test::TestLayoutProvider layout_provider;
  layout_provider.SetDistanceMetric(DISTANCE_BUTTON_MAX_LINKABLE_WIDTH, 200);
  SetLongCancelLabel();

  // Ensure there is no default button since getting a bold font can throw off
  // the cached sizes.
  set_default_button(ui::DIALOG_BUTTON_NONE);

  SetDialogButtons(ui::DIALOG_BUTTON_OK);
  CheckContentsIsSetToPreferredSize();
  const int ok_button_only_width = client_view()->ok_button()->width();

  SetDialogButtons(ui::DIALOG_BUTTON_CANCEL);
  CheckContentsIsSetToPreferredSize();
  const int cancel_button_width = client_view()->cancel_button()->width();
  EXPECT_LT(cancel_button_width, 200);

  // Ensure the single buttons have different preferred widths when alone, and
  // that the Cancel button is bigger (so that it dominates the size).
  EXPECT_GT(cancel_button_width, ok_button_only_width);

  SetDialogButtons(ui::DIALOG_BUTTON_CANCEL | ui::DIALOG_BUTTON_OK);
  CheckContentsIsSetToPreferredSize();

  // Cancel button shouldn't have changed widths.
  EXPECT_EQ(cancel_button_width, client_view()->cancel_button()->width());

  // OK button should now match the bigger, cancel button.
  EXPECT_EQ(cancel_button_width, client_view()->ok_button()->width());

  // But not when the size of the cancel button exceeds the max linkable width.
  layout_provider.SetDistanceMetric(DISTANCE_BUTTON_MAX_LINKABLE_WIDTH, 100);
  EXPECT_GT(cancel_button_width, 100);

  DialogModelChanged();
  CheckContentsIsSetToPreferredSize();
  EXPECT_EQ(ok_button_only_width, client_view()->ok_button()->width());
  layout_provider.SetDistanceMetric(DISTANCE_BUTTON_MAX_LINKABLE_WIDTH, 200);

  // The extra view should also match, if it's a matching button type.
  LabelButton* extra_button = new LabelButton(nullptr, base::string16());
  SetExtraView(extra_button);
  CheckContentsIsSetToPreferredSize();
  EXPECT_EQ(cancel_button_width, extra_button->width());

  // Remove |extra_button| from the View hierarchy so that it can be replaced.
  delete extra_button;

  // Checkbox extends LabelButton, but it should not participate in linking.
  extra_button = new Checkbox(base::string16());
  SetExtraView(extra_button);
  CheckContentsIsSetToPreferredSize();
  EXPECT_NE(cancel_button_width, extra_button->width());

  // Remove |extra_button| from the View hierarchy so that it can be replaced.
  delete extra_button;

  // ImageButton extends Button, but it should not participate in linking. Even
  // without an image, the minimum size (16x14) of the button should be smaller
  // than the dialog buttons.
  ImageButton* image_button = new ImageButton(nullptr);
  SetExtraView(image_button);
  CheckContentsIsSetToPreferredSize();
  EXPECT_NE(cancel_button_width, image_button->width());

  // Remove |image_button| from the View hierarchy so that it can be replaced.
  delete image_button;

  // Non-buttons should always be sized to their preferred size.
  View* boring_view = new StaticSizedView(gfx::Size(20, 20));
  SetExtraView(boring_view);
  CheckContentsIsSetToPreferredSize();
  EXPECT_EQ(20, boring_view->width());
}

TEST_F(DialogClientViewTest, ButtonPosition) {
  constexpr int button_row_inset = 13;
  client_view()->SetButtonRowInsets(gfx::Insets(button_row_inset));
  constexpr int contents_height = 37;
  constexpr int contents_width = 222;
  SetSizeConstraints(gfx::Size(), gfx::Size(contents_width, contents_height),
                     gfx::Size(666, 666));
  SetDialogButtons(ui::DIALOG_BUTTON_OK);
  client_view()->SizeToPreferredSize();
  client_view()->Layout();
  EXPECT_EQ(contents_width - button_row_inset,
            client_view()->ok_button()->bounds().right());
  EXPECT_EQ(contents_height + button_row_inset,
            height() + client_view()->ok_button()->y());
}

// Ensures that the focus of the button remains after a dialog update.
TEST_F(DialogClientViewTest, FocusUpdate) {
  // Test with just an ok button.
  widget()->Show();
  SetDialogButtons(ui::DIALOG_BUTTON_OK);
  EXPECT_FALSE(client_view()->ok_button()->HasFocus());
  client_view()->ok_button()->RequestFocus();  // Set focus.
  EXPECT_TRUE(client_view()->ok_button()->HasFocus());
  DialogModelChanged();
  EXPECT_TRUE(client_view()->ok_button()->HasFocus());
}

// Ensures that the focus of the button remains after a dialog update that
// contains multiple buttons.
TEST_F(DialogClientViewTest, FocusMultipleButtons) {
  // Test with ok and cancel buttons.
  widget()->Show();
  SetDialogButtons(ui::DIALOG_BUTTON_CANCEL | ui::DIALOG_BUTTON_OK);
  EXPECT_FALSE(client_view()->ok_button()->HasFocus());
  EXPECT_FALSE(client_view()->cancel_button()->HasFocus());
  client_view()->cancel_button()->RequestFocus();  // Set focus.
  EXPECT_FALSE(client_view()->ok_button()->HasFocus());
  EXPECT_TRUE(client_view()->cancel_button()->HasFocus());
  DialogModelChanged();
  EXPECT_TRUE(client_view()->cancel_button()->HasFocus());
}

// Ensures that the focus persistence works correctly when buttons are removed.
TEST_F(DialogClientViewTest, FocusChangingButtons) {
  // Start with ok and cancel buttons.
  widget()->Show();
  SetDialogButtons(ui::DIALOG_BUTTON_CANCEL | ui::DIALOG_BUTTON_OK);
  client_view()->cancel_button()->RequestFocus();  // Set focus.
  FocusManager* focus_manager = GetFocusManager();
  EXPECT_EQ(client_view()->cancel_button(), focus_manager->GetFocusedView());

  // Remove buttons.
  SetDialogButtons(ui::DIALOG_BUTTON_NONE);
  EXPECT_EQ(nullptr, focus_manager->GetFocusedView());
}

}  // namespace views
