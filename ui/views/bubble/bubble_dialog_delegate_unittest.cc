// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/bubble/bubble_dialog_delegate.h"

#include <stddef.h>

#include "base/i18n/rtl.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/hit_test.h"
#include "ui/events/event_utils.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/test/test_views.h"
#include "ui/views/test/test_widget_observer.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace views {

namespace {

constexpr int kContentHeight = 200;
constexpr int kContentWidth = 200;

class TestBubbleDialogDelegateView : public BubbleDialogDelegateView {
 public:
  TestBubbleDialogDelegateView(View* anchor_view)
      : BubbleDialogDelegateView(anchor_view, BubbleBorder::TOP_LEFT) {
    view_->SetFocusBehavior(FocusBehavior::ALWAYS);
    AddChildView(view_);
  }
  ~TestBubbleDialogDelegateView() override {}

  // BubbleDialogDelegateView overrides:
  View* GetInitiallyFocusedView() override { return view_; }
  gfx::Size CalculatePreferredSize() const override {
    return gfx::Size(kContentWidth, kContentHeight);
  }
  void AddedToWidget() override {
    if (title_view_)
      GetBubbleFrameView()->SetTitleView(std::move(title_view_));
  }

  base::string16 GetWindowTitle() const override {
    return base::ASCIIToUTF16("TITLE TITLE TITLE");
  }

  bool ShouldShowWindowTitle() const override {
    return should_show_window_title_;
  }

  bool ShouldShowCloseButton() const override {
    return should_show_close_button_;
  }

  int GetDialogButtons() const override { return buttons_; }

  void set_title_view(View* title_view) { title_view_.reset(title_view); }
  void show_close_button() { should_show_close_button_ = true; }
  void hide_buttons() {
    should_show_close_button_ = false;
    buttons_ = ui::DIALOG_BUTTON_NONE;
  }
  void set_should_show_window_title(bool should_show_window_title) {
    should_show_window_title_ = should_show_window_title;
  }

  using BubbleDialogDelegateView::SetAnchorRect;
  using BubbleDialogDelegateView::GetBubbleFrameView;
  using BubbleDialogDelegateView::SizeToContents;

 private:
  View* view_ = new View;
  std::unique_ptr<View> title_view_;
  int buttons_ = ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;
  bool should_show_close_button_ = false;
  bool should_show_window_title_ = true;

  DISALLOW_COPY_AND_ASSIGN(TestBubbleDialogDelegateView);
};

class BubbleDialogDelegateTest : public ViewsTestBase {
 public:
  BubbleDialogDelegateTest() {}
  ~BubbleDialogDelegateTest() override {}

  // Creates and shows a test widget that owns its native widget.
  Widget* CreateTestWidget() {
    Widget* widget = new Widget();
    Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_WINDOW);
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    widget->Init(params);
    widget->Show();
    return widget;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BubbleDialogDelegateTest);
};

}  // namespace

TEST_F(BubbleDialogDelegateTest, CreateDelegate) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  TestBubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  bubble_delegate->set_color(SK_ColorGREEN);
  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  EXPECT_EQ(bubble_delegate, bubble_widget->widget_delegate());
  EXPECT_EQ(bubble_widget, bubble_delegate->GetWidget());
  test::TestWidgetObserver bubble_observer(bubble_widget);
  bubble_widget->Show();

  BubbleBorder* border = bubble_delegate->GetBubbleFrameView()->bubble_border();
  EXPECT_EQ(bubble_delegate->color(), border->background_color());

  EXPECT_FALSE(bubble_observer.widget_closed());
  bubble_widget->CloseNow();
  EXPECT_TRUE(bubble_observer.widget_closed());
}

TEST_F(BubbleDialogDelegateTest, MirrorArrowInRtl) {
  std::string default_locale = base::i18n::GetConfiguredLocale();
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  for (bool rtl : {false, true}) {
    base::i18n::SetICUDefaultLocale(rtl ? "he" : "en");
    EXPECT_EQ(rtl, base::i18n::IsRTL());
    for (bool mirror : {false, true}) {
      TestBubbleDialogDelegateView* bubble =
          new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
      bubble->set_mirror_arrow_in_rtl(mirror);
      BubbleDialogDelegateView::CreateBubble(bubble);
      EXPECT_EQ(rtl && mirror ? BubbleBorder::horizontal_mirror(bubble->arrow())
                              : bubble->arrow(),
                bubble->GetBubbleFrameView()->bubble_border()->arrow());
    }
  }
  base::i18n::SetICUDefaultLocale(default_locale);
}

TEST_F(BubbleDialogDelegateTest, CloseAnchorWidget) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  BubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  // Preventing close on deactivate should not prevent closing with the anchor.
  bubble_delegate->set_close_on_deactivate(false);
  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  EXPECT_EQ(bubble_delegate, bubble_widget->widget_delegate());
  EXPECT_EQ(bubble_widget, bubble_delegate->GetWidget());
  EXPECT_EQ(anchor_widget.get(), bubble_delegate->anchor_widget());
  test::TestWidgetObserver bubble_observer(bubble_widget);
  EXPECT_FALSE(bubble_observer.widget_closed());

  bubble_widget->Show();
  EXPECT_EQ(anchor_widget.get(), bubble_delegate->anchor_widget());
  EXPECT_FALSE(bubble_observer.widget_closed());

  // TODO(msw): Remove activation hack to prevent bookkeeping errors in:
  //            aura::test::TestActivationClient::OnWindowDestroyed().
  std::unique_ptr<Widget> smoke_and_mirrors_widget(CreateTestWidget());
  EXPECT_FALSE(bubble_observer.widget_closed());

  // Ensure that closing the anchor widget also closes the bubble itself.
  anchor_widget->CloseNow();
  EXPECT_TRUE(bubble_observer.widget_closed());
}

// This test checks that the bubble delegate is capable to handle an early
// destruction of the used anchor view. (Animations and delayed closure of the
// bubble will call upon the anchor view to get its location).
TEST_F(BubbleDialogDelegateTest, CloseAnchorViewTest) {
  // Create an anchor widget and add a view to be used as an anchor view.
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  std::unique_ptr<View> anchor_view(new View());
  anchor_widget->GetContentsView()->AddChildView(anchor_view.get());
  TestBubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_view.get());
  // Prevent flakes by avoiding closing on activation changes.
  bubble_delegate->set_close_on_deactivate(false);
  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);

  // Check that the anchor view is correct and set up an anchor view rect.
  // Make sure that this rect will get ignored (as long as the anchor view is
  // attached).
  EXPECT_EQ(anchor_view.get(), bubble_delegate->GetAnchorView());
  const gfx::Rect set_anchor_rect = gfx::Rect(10, 10, 100, 100);
  bubble_delegate->SetAnchorRect(set_anchor_rect);
  const gfx::Rect view_rect = bubble_delegate->GetAnchorRect();
  EXPECT_NE(view_rect.ToString(), set_anchor_rect.ToString());

  // Create the bubble.
  bubble_widget->Show();
  EXPECT_EQ(anchor_widget.get(), bubble_delegate->anchor_widget());

  // Remove now the anchor view and make sure that the original found rect
  // is still kept, so that the bubble does not jump when the view gets deleted.
  anchor_widget->GetContentsView()->RemoveChildView(anchor_view.get());
  anchor_view.reset();
  EXPECT_EQ(NULL, bubble_delegate->GetAnchorView());
  EXPECT_EQ(view_rect.ToString(), bubble_delegate->GetAnchorRect().ToString());
}

// Testing that a move of the anchor view will lead to new bubble locations.
TEST_F(BubbleDialogDelegateTest, TestAnchorRectMovesWithViewTest) {
  // Create an anchor widget and add a view to be used as anchor view.
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  TestBubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  BubbleDialogDelegateView::CreateBubble(bubble_delegate);

  anchor_widget->GetContentsView()->SetBounds(10, 10, 100, 100);
  const gfx::Rect view_rect = bubble_delegate->GetAnchorRect();

  anchor_widget->GetContentsView()->SetBounds(20, 10, 100, 100);
  const gfx::Rect view_rect_2 = bubble_delegate->GetAnchorRect();
  EXPECT_NE(view_rect.ToString(), view_rect_2.ToString());
}

TEST_F(BubbleDialogDelegateTest, ResetAnchorWidget) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  BubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());

  // Make sure the bubble widget is parented to a widget other than the anchor
  // widget so that closing the anchor widget does not close the bubble widget.
  std::unique_ptr<Widget> parent_widget(CreateTestWidget());
  bubble_delegate->set_parent_window(parent_widget->GetNativeView());
  // Preventing close on deactivate should not prevent closing with the parent.
  bubble_delegate->set_close_on_deactivate(false);
  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  EXPECT_EQ(bubble_delegate, bubble_widget->widget_delegate());
  EXPECT_EQ(bubble_widget, bubble_delegate->GetWidget());
  EXPECT_EQ(anchor_widget.get(), bubble_delegate->anchor_widget());
  test::TestWidgetObserver bubble_observer(bubble_widget);
  EXPECT_FALSE(bubble_observer.widget_closed());

  // Showing and hiding the bubble widget should have no effect on its anchor.
  bubble_widget->Show();
  EXPECT_EQ(anchor_widget.get(), bubble_delegate->anchor_widget());
  bubble_widget->Hide();
  EXPECT_EQ(anchor_widget.get(), bubble_delegate->anchor_widget());

  // Ensure that closing the anchor widget clears the bubble's reference to that
  // anchor widget, but the bubble itself does not close.
  anchor_widget->CloseNow();
  EXPECT_NE(anchor_widget.get(), bubble_delegate->anchor_widget());
  EXPECT_FALSE(bubble_observer.widget_closed());

  // TODO(msw): Remove activation hack to prevent bookkeeping errors in:
  //            aura::test::TestActivationClient::OnWindowDestroyed().
  std::unique_ptr<Widget> smoke_and_mirrors_widget(CreateTestWidget());
  EXPECT_FALSE(bubble_observer.widget_closed());

  // Ensure that closing the parent widget also closes the bubble itself.
  parent_widget->CloseNow();
  EXPECT_TRUE(bubble_observer.widget_closed());
}

TEST_F(BubbleDialogDelegateTest, InitiallyFocusedView) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  BubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  bubble_widget->Show();
  EXPECT_EQ(bubble_delegate->GetInitiallyFocusedView(),
            bubble_widget->GetFocusManager()->GetFocusedView());
  bubble_widget->CloseNow();
}

TEST_F(BubbleDialogDelegateTest, NonClientHitTest) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  TestBubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  BubbleFrameView* frame = bubble_delegate->GetBubbleFrameView();
  const int border = frame->bubble_border()->GetBorderThickness();

  struct {
    const int point;
    const int hit;
  } cases[] = {
      {border, HTNOWHERE}, {border + 60, HTCLIENT}, {1000, HTNOWHERE},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    gfx::Point point(cases[i].point, cases[i].point);
    EXPECT_EQ(cases[i].hit, frame->NonClientHitTest(point))
        << " with border: " << border << ", at point " << cases[i].point;
  }
}

TEST_F(BubbleDialogDelegateTest, VisibleWhenAnchorWidgetBoundsChanged) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  BubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  test::TestWidgetObserver bubble_observer(bubble_widget);
  EXPECT_FALSE(bubble_observer.widget_closed());

  bubble_widget->Show();
  EXPECT_TRUE(bubble_widget->IsVisible());
  anchor_widget->SetBounds(gfx::Rect(10, 10, 100, 100));
  EXPECT_TRUE(bubble_widget->IsVisible());
}

// Test that setting WidgetDelegate::set_can_activate() to false makes the
// widget created via BubbleDialogDelegateView::CreateBubble() not activatable.
TEST_F(BubbleDialogDelegateTest, NotActivatable) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  BubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  bubble_delegate->set_can_activate(false);
  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  bubble_widget->Show();
  EXPECT_FALSE(bubble_widget->CanActivate());
}

TEST_F(BubbleDialogDelegateTest, CloseMethods) {
  {
    std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
    BubbleDialogDelegateView* bubble_delegate =
        new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
    bubble_delegate->set_close_on_deactivate(true);
    Widget* bubble_widget =
        BubbleDialogDelegateView::CreateBubble(bubble_delegate);
    bubble_widget->Show();
    anchor_widget->Activate();
    EXPECT_TRUE(bubble_widget->IsClosed());
  }

  {
    std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
    BubbleDialogDelegateView* bubble_delegate =
        new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
    Widget* bubble_widget =
        BubbleDialogDelegateView::CreateBubble(bubble_delegate);
    bubble_widget->Show();

    ui::KeyEvent escape_event(ui::ET_KEY_PRESSED, ui::VKEY_ESCAPE, ui::EF_NONE);
    bubble_widget->OnKeyEvent(&escape_event);
    EXPECT_TRUE(bubble_widget->IsClosed());
  }

  {
    std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
    TestBubbleDialogDelegateView* bubble_delegate =
        new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
    Widget* bubble_widget =
        BubbleDialogDelegateView::CreateBubble(bubble_delegate);
    bubble_widget->Show();
    BubbleFrameView* frame_view = bubble_delegate->GetBubbleFrameView();
    Button* close_button = frame_view->close_;
    ASSERT_TRUE(close_button);
    frame_view->ButtonPressed(
        close_button,
        ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE));
    EXPECT_TRUE(bubble_widget->IsClosed());
  }
}

TEST_F(BubbleDialogDelegateTest, CustomTitle) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  TestBubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  constexpr int kTitleHeight = 20;
  View* title_view = new StaticSizedView(gfx::Size(10, kTitleHeight));
  bubble_delegate->set_title_view(title_view);
  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  bubble_widget->Show();

  BubbleFrameView* bubble_frame = static_cast<BubbleFrameView*>(
      bubble_widget->non_client_view()->frame_view());
  EXPECT_EQ(title_view, bubble_frame->title());
  EXPECT_EQ(bubble_frame, title_view->parent());
  // Title takes up the whole bubble width when there's no icon or close button.
  EXPECT_EQ(bubble_delegate->width(), title_view->size().width());
  EXPECT_EQ(kTitleHeight, title_view->size().height());

  bubble_delegate->show_close_button();
  bubble_frame->ResetWindowControls();
  bubble_frame->Layout();

  Button* close_button = bubble_frame->GetCloseButtonForTest();
  // Title moves over for the close button.
  EXPECT_EQ(close_button->x() - LayoutProvider::Get()->GetDistanceMetric(
                                    DISTANCE_CLOSE_BUTTON_MARGIN),
            title_view->bounds().right());

  LayoutProvider* provider = LayoutProvider::Get();
  const gfx::Insets content_margins =
      provider->GetDialogInsetsForContentType(views::TEXT, views::TEXT);
  const gfx::Insets title_margins =
      provider->GetInsetsMetric(INSETS_DIALOG_TITLE);
  EXPECT_EQ(content_margins, bubble_delegate->margins());
  // Note there is no title_margins() accessor (it should not be customizable).

  // To perform checks on the precise size, first hide the dialog buttons so the
  // calculations are simpler (e.g. platform font discrepancies can be ignored).
  bubble_delegate->hide_buttons();
  bubble_frame->ResetWindowControls();
  bubble_delegate->DialogModelChanged();
  bubble_delegate->SizeToContents();

  // Use GetContentsBounds() to exclude the bubble border, which can change per
  // platform.
  gfx::Rect frame_size = bubble_frame->GetContentsBounds();
  EXPECT_EQ(content_margins.height() + kContentHeight + title_margins.height() +
                kTitleHeight,
            frame_size.height());
  EXPECT_EQ(content_margins.width() + kContentWidth, frame_size.width());

  // Set the title preferred size to 0. The bubble frame makes fewer assumptions
  // about custom title views, so there should still be margins for it while the
  // WidgetDelegate says it should be shown, even if its preferred size is zero.
  title_view->SetPreferredSize(gfx::Size());
  bubble_widget->UpdateWindowTitle();
  bubble_delegate->SizeToContents();
  frame_size = bubble_frame->GetContentsBounds();
  EXPECT_EQ(content_margins.height() + kContentHeight + title_margins.height(),
            frame_size.height());
  EXPECT_EQ(content_margins.width() + kContentWidth, frame_size.width());

  // Now hide the title properly. The margins should also disappear.
  bubble_delegate->set_should_show_window_title(false);
  bubble_widget->UpdateWindowTitle();
  bubble_delegate->SizeToContents();
  frame_size = bubble_frame->GetContentsBounds();
  EXPECT_EQ(content_margins.height() + kContentHeight, frame_size.height());
  EXPECT_EQ(content_margins.width() + kContentWidth, frame_size.width());
}

// Ensure the BubbleFrameView correctly resizes when the title is provided by a
// StyledLabel.
TEST_F(BubbleDialogDelegateTest, StyledLabelTitle) {
  std::unique_ptr<Widget> anchor_widget(CreateTestWidget());
  TestBubbleDialogDelegateView* bubble_delegate =
      new TestBubbleDialogDelegateView(anchor_widget->GetContentsView());
  StyledLabel* title_view = new StyledLabel(base::ASCIIToUTF16("123"), nullptr);
  bubble_delegate->set_title_view(title_view);

  Widget* bubble_widget =
      BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  bubble_widget->Show();

  const gfx::Size size_before_new_title =
      bubble_widget->GetWindowBoundsInScreen().size();
  title_view->SetText(base::ASCIIToUTF16("12"));
  bubble_delegate->SizeToContents();

  // A shorter title should change nothing, since both will be within the
  // minimum dialog width.
  EXPECT_EQ(size_before_new_title,
            bubble_widget->GetWindowBoundsInScreen().size());

  title_view->SetText(base::UTF8ToUTF16(std::string(200, '0')));
  bubble_delegate->SizeToContents();

  // A (much) longer title should increase the height, but not the width.
  EXPECT_EQ(size_before_new_title.width(),
            bubble_widget->GetWindowBoundsInScreen().width());
  EXPECT_LT(size_before_new_title.height(),
            bubble_widget->GetWindowBoundsInScreen().height());
}

}  // namespace views
