// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/custom_frame_view_ash.h"

#include <memory>

#include "ash/accelerators/accelerator_controller.h"
#include "ash/frame/caption_buttons/frame_caption_button.h"
#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/frame/default_frame_header.h"
#include "ash/frame/header_view.h"
#include "ash/frame/wide_frame_view.h"
#include "ash/public/cpp/ash_layout_constants.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller_test_api.h"
#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_delegate.h"
#include "ash/wm/wm_event.h"
#include "base/containers/flat_set.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_targeter.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

// A views::WidgetDelegate which uses a CustomFrameViewAsh.
class CustomFrameTestWidgetDelegate : public views::WidgetDelegateView {
 public:
  CustomFrameTestWidgetDelegate() = default;
  ~CustomFrameTestWidgetDelegate() override = default;

  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override {
    custom_frame_view_ = new CustomFrameViewAsh(widget);
    return custom_frame_view_;
  }

  int GetCustomFrameViewTopBorderHeight() {
    return custom_frame_view_->NonClientTopBorderHeight();
  }

  CustomFrameViewAsh* custom_frame_view() const { return custom_frame_view_; }

  HeaderView* header_view() const { return custom_frame_view_->header_view_; }

 private:
  // Not owned.
  CustomFrameViewAsh* custom_frame_view_;

  DISALLOW_COPY_AND_ASSIGN(CustomFrameTestWidgetDelegate);
};

class TestWidgetConstraintsDelegate : public CustomFrameTestWidgetDelegate {
 public:
  TestWidgetConstraintsDelegate() = default;
  ~TestWidgetConstraintsDelegate() override = default;

  // views::View:
  gfx::Size GetMinimumSize() const override { return minimum_size_; }

  gfx::Size GetMaximumSize() const override { return maximum_size_; }

  views::View* GetContentsView() override {
    // Set this instance as the contents view so that the maximum and minimum
    // size constraints will be used.
    return this;
  }

  // views::WidgetDelegate:
  bool CanMaximize() const override { return true; }

  bool CanMinimize() const override { return true; }

  void set_minimum_size(const gfx::Size& min_size) { minimum_size_ = min_size; }

  void set_maximum_size(const gfx::Size& max_size) { maximum_size_ = max_size; }

  const gfx::Rect& GetFrameCaptionButtonContainerViewBounds() {
    return custom_frame_view()
        ->GetFrameCaptionButtonContainerViewForTest()
        ->bounds();
  }

  void EndFrameCaptionButtonContainerViewAnimations() {
    FrameCaptionButtonContainerView::TestApi test(
        custom_frame_view()->GetFrameCaptionButtonContainerViewForTest());
    test.EndAnimations();
  }

  int GetTitleBarHeight() const {
    return custom_frame_view()->NonClientTopBorderHeight();
  }

 private:
  gfx::Size minimum_size_;
  gfx::Size maximum_size_;

  DISALLOW_COPY_AND_ASSIGN(TestWidgetConstraintsDelegate);
};

using CustomFrameViewAshTest = AshTestBase;

// Verifies the client view is not placed at a y location of 0.
TEST_F(CustomFrameViewAshTest, ClientViewCorrectlyPlaced) {
  std::unique_ptr<views::Widget> widget =
      CreateTestWidget(new CustomFrameTestWidgetDelegate);
  EXPECT_NE(0, widget->client_view()->bounds().y());
}

// Test that the height of the header is correct upon initially displaying
// the widget.
TEST_F(CustomFrameViewAshTest, HeaderHeight) {
  CustomFrameTestWidgetDelegate* delegate = new CustomFrameTestWidgetDelegate;

  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  // The header should have enough room for the window controls. The
  // header/content separator line overlays the window controls.
  EXPECT_EQ(GetAshLayoutSize(AshLayoutSize::kNonBrowserCaption).height(),
            delegate->custom_frame_view()->GetHeaderView()->height());
}

// Verify that CustomFrameViewAsh returns the correct minimum and maximum frame
// sizes when the client view does not specify any size constraints.
TEST_F(CustomFrameViewAshTest, NoSizeConstraints) {
  TestWidgetConstraintsDelegate* delegate = new TestWidgetConstraintsDelegate;
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  CustomFrameViewAsh* custom_frame_view = delegate->custom_frame_view();
  gfx::Size min_frame_size = custom_frame_view->GetMinimumSize();
  gfx::Size max_frame_size = custom_frame_view->GetMaximumSize();

  EXPECT_EQ(delegate->GetTitleBarHeight(), min_frame_size.height());

  // A width and height constraint of 0 denotes unbounded.
  EXPECT_EQ(0, max_frame_size.width());
  EXPECT_EQ(0, max_frame_size.height());
}

// Verify that CustomFrameViewAsh returns the correct minimum and maximum frame
// sizes when the client view specifies size constraints.
TEST_F(CustomFrameViewAshTest, MinimumAndMaximumSize) {
  gfx::Size min_client_size(500, 500);
  gfx::Size max_client_size(800, 800);
  TestWidgetConstraintsDelegate* delegate = new TestWidgetConstraintsDelegate;
  delegate->set_minimum_size(min_client_size);
  delegate->set_maximum_size(max_client_size);
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  CustomFrameViewAsh* custom_frame_view = delegate->custom_frame_view();
  gfx::Size min_frame_size = custom_frame_view->GetMinimumSize();
  gfx::Size max_frame_size = custom_frame_view->GetMaximumSize();

  EXPECT_EQ(min_client_size.width(), min_frame_size.width());
  EXPECT_EQ(max_client_size.width(), max_frame_size.width());
  EXPECT_EQ(min_client_size.height() + delegate->GetTitleBarHeight(),
            min_frame_size.height());
  EXPECT_EQ(max_client_size.height() + delegate->GetTitleBarHeight(),
            max_frame_size.height());
}

// Verify that CustomFrameViewAsh returns the correct minimum frame size when
// the kMinimumSize property is set.
TEST_F(CustomFrameViewAshTest, HonorsMinimumSizeProperty) {
  const gfx::Size min_client_size(500, 500);
  TestWidgetConstraintsDelegate* delegate = new TestWidgetConstraintsDelegate;
  delegate->set_minimum_size(min_client_size);
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  // Update the native window's minimum size property.
  const gfx::Size min_window_size(600, 700);
  widget->GetNativeWindow()->SetProperty(aura::client::kMinimumSize,
                                         new gfx::Size(min_window_size));

  CustomFrameViewAsh* custom_frame_view = delegate->custom_frame_view();
  const gfx::Size min_frame_size = custom_frame_view->GetMinimumSize();

  EXPECT_EQ(min_window_size, min_frame_size);
}

// Verify that CustomFrameViewAsh updates the avatar icon based on the
// avatar icon window property.
TEST_F(CustomFrameViewAshTest, AvatarIcon) {
  TestWidgetConstraintsDelegate* delegate = new TestWidgetConstraintsDelegate;
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  CustomFrameViewAsh* custom_frame_view = delegate->custom_frame_view();
  EXPECT_FALSE(custom_frame_view->GetAvatarIconViewForTest());

  // Avatar image becomes available.
  widget->GetNativeWindow()->SetProperty(
      aura::client::kAvatarIconKey,
      new gfx::ImageSkia(gfx::test::CreateImage(27, 27).AsImageSkia()));
  EXPECT_TRUE(custom_frame_view->GetAvatarIconViewForTest());

  // Avatar image is gone; the ImageView for the avatar icon should be
  // removed.
  widget->GetNativeWindow()->ClearProperty(aura::client::kAvatarIconKey);
  EXPECT_FALSE(custom_frame_view->GetAvatarIconViewForTest());
}

// The visibility of the size button is updated when tablet mode is toggled.
// Verify that the layout of the HeaderView is updated for the size button's
// new visibility.
TEST_F(CustomFrameViewAshTest, HeaderViewNotifiedOfChildSizeChange) {
  TestWidgetConstraintsDelegate* delegate = new TestWidgetConstraintsDelegate;
  std::unique_ptr<views::Widget> widget = CreateTestWidget(
      delegate, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 400, 500));

  const gfx::Rect initial =
      delegate->GetFrameCaptionButtonContainerViewBounds();
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  delegate->EndFrameCaptionButtonContainerViewAnimations();
  const gfx::Rect tablet_mode_bounds =
      delegate->GetFrameCaptionButtonContainerViewBounds();
  EXPECT_GT(initial.width(), tablet_mode_bounds.width());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  delegate->EndFrameCaptionButtonContainerViewAnimations();
  const gfx::Rect after_restore =
      delegate->GetFrameCaptionButtonContainerViewBounds();
  EXPECT_EQ(initial, after_restore);
}

// Verify that when in tablet mode with a maximized window, the height of the
// header is zero.
TEST_F(CustomFrameViewAshTest, FrameHiddenInTabletModeForMaximizedWindows) {
  CustomFrameTestWidgetDelegate* delegate = new CustomFrameTestWidgetDelegate;
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);
  widget->Maximize();

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_EQ(0, delegate->GetCustomFrameViewTopBorderHeight());
}

// Verify that when in tablet mode with a non maximized window, the height of
// the header is non zero.
TEST_F(CustomFrameViewAshTest, FrameShownInTabletModeForNonMaximizedWindows) {
  auto* delegate = new CustomFrameTestWidgetDelegate();
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_EQ(GetAshLayoutSize(AshLayoutSize::kNonBrowserCaption).height(),
            delegate->GetCustomFrameViewTopBorderHeight());
}

// Verify that if originally in fullscreen mode, and enter tablet mode, the
// height of the header remains zero.
TEST_F(CustomFrameViewAshTest,
       FrameRemainsHiddenInTabletModeWhenTogglingFullscreen) {
  CustomFrameTestWidgetDelegate* delegate = new CustomFrameTestWidgetDelegate;
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  widget->SetFullscreen(true);
  EXPECT_EQ(0, delegate->GetCustomFrameViewTopBorderHeight());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_EQ(0, delegate->GetCustomFrameViewTopBorderHeight());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_EQ(0, delegate->GetCustomFrameViewTopBorderHeight());
}

TEST_F(CustomFrameViewAshTest, OpeningAppsInTabletMode) {
  auto* delegate = new TestWidgetConstraintsDelegate;
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);
  widget->Maximize();

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_EQ(0, delegate->GetCustomFrameViewTopBorderHeight());

  // Verify that after minimizing and showing the widget, the height of the
  // header is zero.
  widget->Minimize();
  widget->Show();
  EXPECT_TRUE(widget->IsMaximized());
  EXPECT_EQ(0, delegate->GetCustomFrameViewTopBorderHeight());

  // Verify that when we toggle maximize, the header is shown. For example,
  // maximized can be toggled in tablet mode by using the accessibility
  // keyboard.
  wm::WMEvent event(wm::WM_EVENT_TOGGLE_MAXIMIZE);
  wm::GetWindowState(widget->GetNativeWindow())->OnWMEvent(&event);
  EXPECT_EQ(0, delegate->GetCustomFrameViewTopBorderHeight());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_EQ(GetAshLayoutSize(AshLayoutSize::kNonBrowserCaption).height(),
            delegate->GetCustomFrameViewTopBorderHeight());
}

// Test if creating a new window in tablet mode uses maximzied state
// and immersive mode.
TEST_F(CustomFrameViewAshTest, GetPreferredOnScreenHeightInTabletMaximzied) {
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  auto* delegate = new TestWidgetConstraintsDelegate;
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);
  auto* frame_view = static_cast<ash::CustomFrameViewAsh*>(
      widget->non_client_view()->frame_view());
  auto* header_view = static_cast<HeaderView*>(frame_view->GetHeaderView());
  ASSERT_TRUE(widget->IsMaximized());
  EXPECT_TRUE(header_view->in_immersive_mode());
  static_cast<ImmersiveFullscreenControllerDelegate*>(header_view)
      ->SetVisibleFraction(0.5);
  // The height should be ~(33 *.5)
  EXPECT_NEAR(16, header_view->GetPreferredOnScreenHeight(), 1);
  static_cast<ImmersiveFullscreenControllerDelegate*>(header_view)
      ->SetVisibleFraction(0.0);
  EXPECT_EQ(0, header_view->GetPreferredOnScreenHeight());
}

// Verify windows that are minimized and then entered into tablet mode will have
// no header when unminimized in tablet mode.
TEST_F(CustomFrameViewAshTest, MinimizedWindowsInTabletMode) {
  std::unique_ptr<views::Widget> widget =
      CreateTestWidget(new CustomFrameTestWidgetDelegate);
  widget->GetNativeWindow()->SetProperty(aura::client::kResizeBehaviorKey,
                                         ui::mojom::kResizeBehaviorCanMaximize);
  widget->Maximize();
  widget->Minimize();
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  widget->Show();
  EXPECT_EQ(widget->non_client_view()->bounds(),
            widget->client_view()->bounds());
}

TEST_F(CustomFrameViewAshTest, HeaderVisibilityInOverviewMode) {
  auto* delegate = new CustomFrameTestWidgetDelegate();
  std::unique_ptr<views::Widget> widget = CreateTestWidget(
      delegate, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 400, 500));

  // Verify the header is not painted in overview mode and painted when not in
  // overview mode.
  Shell::Get()->window_selector_controller()->ToggleOverview();
  EXPECT_FALSE(delegate->header_view()->should_paint());

  Shell::Get()->window_selector_controller()->ToggleOverview();
  EXPECT_TRUE(delegate->header_view()->should_paint());
}

TEST_F(CustomFrameViewAshTest, HeaderVisibilityInSplitview) {
  auto create_widget = [this](CustomFrameTestWidgetDelegate* delegate) {
    std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);
    // Windows need to be resizable and maximizable to be used in splitview.
    widget->GetNativeWindow()->SetProperty(
        aura::client::kResizeBehaviorKey,
        ui::mojom::kResizeBehaviorCanMaximize |
            ui::mojom::kResizeBehaviorCanResize);
    return widget;
  };

  auto* delegate1 = new CustomFrameTestWidgetDelegate();
  auto widget1 = create_widget(delegate1);
  auto* delegate2 = new CustomFrameTestWidgetDelegate();
  auto widget2 = create_widget(delegate2);

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  // Verify that when one window is snapped, the header is drawn for the snapped
  // window, but not drawn for the window still in overview.
  Shell::Get()->window_selector_controller()->ToggleOverview();
  Shell::Get()->split_view_controller()->SnapWindow(widget1->GetNativeWindow(),
                                                    SplitViewController::LEFT);
  EXPECT_TRUE(delegate1->header_view()->should_paint());
  EXPECT_EQ(0, delegate1->GetCustomFrameViewTopBorderHeight());
  EXPECT_FALSE(delegate2->header_view()->should_paint());

  // Verify that when both windows are snapped, the header is drawn for both.
  Shell::Get()->split_view_controller()->SnapWindow(widget2->GetNativeWindow(),
                                                    SplitViewController::RIGHT);
  EXPECT_TRUE(delegate1->header_view()->should_paint());
  EXPECT_TRUE(delegate2->header_view()->should_paint());
  EXPECT_EQ(0, delegate1->GetCustomFrameViewTopBorderHeight());
  EXPECT_EQ(0, delegate2->GetCustomFrameViewTopBorderHeight());

  // Toggle overview mode so we return back to left snapped mode. Verify that
  // the header is again drawn for the snapped window, but not for the unsnapped
  // window.
  Shell::Get()->window_selector_controller()->ToggleOverview();
  ASSERT_EQ(SplitViewController::LEFT_SNAPPED,
            Shell::Get()->split_view_controller()->state());
  EXPECT_TRUE(delegate1->header_view()->should_paint());
  EXPECT_EQ(0, delegate1->GetCustomFrameViewTopBorderHeight());
  EXPECT_FALSE(delegate2->header_view()->should_paint());

  Shell::Get()->split_view_controller()->EndSplitView();
}

namespace {

class TestTarget : public ui::AcceleratorTarget {
 public:
  TestTarget() = default;
  ~TestTarget() override = default;

  size_t count() const { return count_; }

  // Overridden from ui::AcceleratorTarget:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override {
    ++count_;
    return true;
  }

  bool CanHandleAccelerators() const override { return true; }

 private:
  size_t count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestTarget);
};

class TestButtonModel : public CaptionButtonModel {
 public:
  TestButtonModel() = default;
  ~TestButtonModel() override = default;

  void set_zoom_mode(bool zoom_mode) { zoom_mode_ = zoom_mode; }

  void SetVisible(CaptionButtonIcon type, bool visible) {
    if (visible)
      visible_buttons_.insert(type);
    else
      visible_buttons_.erase(type);
  }

  void SetEnabled(CaptionButtonIcon type, bool enabled) {
    if (enabled)
      enabled_buttons_.insert(type);
    else
      enabled_buttons_.erase(type);
  }

  // CaptionButtonModel::
  bool IsVisible(CaptionButtonIcon type) const override {
    return visible_buttons_.count(type);
  }
  bool IsEnabled(CaptionButtonIcon type) const override {
    return enabled_buttons_.count(type);
  }
  bool InZoomMode() const override { return zoom_mode_; }

 private:
  base::flat_set<CaptionButtonIcon> visible_buttons_;
  base::flat_set<CaptionButtonIcon> enabled_buttons_;
  bool zoom_mode_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestButtonModel);
};

}  // namespace

TEST_F(CustomFrameViewAshTest, BackButton) {
  ash::AcceleratorController* controller =
      ash::Shell::Get()->accelerator_controller();
  std::unique_ptr<TestButtonModel> model = std::make_unique<TestButtonModel>();
  TestButtonModel* model_ptr = model.get();

  auto* delegate = new CustomFrameTestWidgetDelegate();
  std::unique_ptr<views::Widget> widget = CreateTestWidget(
      delegate, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 400, 500));

  ui::Accelerator accelerator_back_press(ui::VKEY_BROWSER_BACK, ui::EF_NONE);
  accelerator_back_press.set_key_state(ui::Accelerator::KeyState::PRESSED);
  TestTarget target_back_press;
  controller->Register({accelerator_back_press}, &target_back_press);

  ui::Accelerator accelerator_back_release(ui::VKEY_BROWSER_BACK, ui::EF_NONE);
  accelerator_back_release.set_key_state(ui::Accelerator::KeyState::RELEASED);
  TestTarget target_back_release;
  controller->Register({accelerator_back_release}, &target_back_release);

  CustomFrameViewAsh* custom_frame_view = delegate->custom_frame_view();
  custom_frame_view->SetCaptionButtonModel(std::move(model));

  HeaderView* header_view =
      static_cast<HeaderView*>(custom_frame_view->GetHeaderView());
  EXPECT_FALSE(header_view->GetBackButton());
  model_ptr->SetVisible(CAPTION_BUTTON_ICON_BACK, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(header_view->GetBackButton());
  EXPECT_FALSE(header_view->GetBackButton()->enabled());

  // Back button is disabled, so clicking on it should not should
  // generate back key sequence.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.MoveMouseTo(
      header_view->GetBackButton()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_EQ(0u, target_back_press.count());
  EXPECT_EQ(0u, target_back_release.count());

  model_ptr->SetEnabled(CAPTION_BUTTON_ICON_BACK, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(header_view->GetBackButton());
  EXPECT_TRUE(header_view->GetBackButton()->enabled());

  // Back button is now enabled, so clicking on it should generate
  // back key sequence.
  generator.MoveMouseTo(
      header_view->GetBackButton()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_EQ(1u, target_back_press.count());
  EXPECT_EQ(1u, target_back_release.count());

  model_ptr->SetVisible(CAPTION_BUTTON_ICON_BACK, false);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_FALSE(header_view->GetBackButton());
}

// Make sure that client view occupies the entire window when the
// frame is hidden.
TEST_F(CustomFrameViewAshTest, FrameVisibility) {
  CustomFrameTestWidgetDelegate* delegate = new CustomFrameTestWidgetDelegate;
  gfx::Rect window_bounds(10, 10, 200, 100);
  std::unique_ptr<views::Widget> widget = CreateTestWidget(
      delegate, kShellWindowId_DefaultContainer, window_bounds);

  // The height is smaller by the top border height.
  gfx::Size client_bounds(200, 68);
  CustomFrameViewAsh* custom_frame_view = delegate->custom_frame_view();
  EXPECT_EQ(client_bounds, widget->client_view()->GetLocalBounds().size());

  custom_frame_view->SetVisible(false);
  widget->GetRootView()->Layout();
  EXPECT_EQ(gfx::Size(200, 100),
            widget->client_view()->GetLocalBounds().size());
  EXPECT_FALSE(widget->non_client_view()->frame_view()->visible());
  EXPECT_EQ(window_bounds,
            custom_frame_view->GetClientBoundsForWindowBounds(window_bounds));

  custom_frame_view->SetVisible(true);
  widget->GetRootView()->Layout();
  EXPECT_EQ(client_bounds, widget->client_view()->GetLocalBounds().size());
  EXPECT_TRUE(widget->non_client_view()->frame_view()->visible());
  EXPECT_EQ(32, delegate->GetCustomFrameViewTopBorderHeight());
  EXPECT_EQ(gfx::Rect(gfx::Point(10, 42), client_bounds),
            custom_frame_view->GetClientBoundsForWindowBounds(window_bounds));
}

TEST_F(CustomFrameViewAshTest, CustomButtonModel) {
  std::unique_ptr<TestButtonModel> model = std::make_unique<TestButtonModel>();
  TestButtonModel* model_ptr = model.get();

  auto* delegate = new CustomFrameTestWidgetDelegate();
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  CustomFrameViewAsh* custom_frame_view = delegate->custom_frame_view();
  custom_frame_view->SetCaptionButtonModel(std::move(model));

  HeaderView* header_view =
      static_cast<HeaderView*>(custom_frame_view->GetHeaderView());
  FrameCaptionButtonContainerView::TestApi test_api(
      header_view->caption_button_container());

  // CLOSE buttion is always visible and enabled.
  EXPECT_TRUE(test_api.close_button());
  EXPECT_TRUE(test_api.close_button()->visible());
  EXPECT_TRUE(test_api.close_button()->enabled());

  EXPECT_FALSE(test_api.minimize_button()->visible());
  EXPECT_FALSE(test_api.size_button()->visible());
  EXPECT_FALSE(test_api.menu_button()->visible());

  // Back button
  model_ptr->SetVisible(CAPTION_BUTTON_ICON_BACK, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(header_view->GetBackButton()->visible());
  EXPECT_FALSE(header_view->GetBackButton()->enabled());

  model_ptr->SetEnabled(CAPTION_BUTTON_ICON_BACK, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(header_view->GetBackButton()->enabled());

  // size button
  model_ptr->SetVisible(CAPTION_BUTTON_ICON_MAXIMIZE_RESTORE, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(test_api.size_button()->visible());
  EXPECT_FALSE(test_api.size_button()->enabled());

  model_ptr->SetEnabled(CAPTION_BUTTON_ICON_MAXIMIZE_RESTORE, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(test_api.size_button()->enabled());

  // minimize button
  model_ptr->SetVisible(CAPTION_BUTTON_ICON_MINIMIZE, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(test_api.minimize_button()->visible());
  EXPECT_FALSE(test_api.minimize_button()->enabled());

  model_ptr->SetEnabled(CAPTION_BUTTON_ICON_MINIMIZE, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(test_api.minimize_button()->enabled());

  // menu button
  model_ptr->SetVisible(CAPTION_BUTTON_ICON_MENU, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(test_api.menu_button()->visible());
  EXPECT_FALSE(test_api.menu_button()->enabled());

  model_ptr->SetEnabled(CAPTION_BUTTON_ICON_MENU, true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_TRUE(test_api.menu_button()->enabled());

  // zoom button
  EXPECT_STREQ(kWindowControlMaximizeIcon.name,
               test_api.size_button()->icon_definition_for_test()->name);
  model_ptr->set_zoom_mode(true);
  custom_frame_view->SizeConstraintsChanged();
  EXPECT_STREQ(kWindowControlZoomIcon.name,
               test_api.size_button()->icon_definition_for_test()->name);
  widget->Maximize();
  EXPECT_STREQ(kWindowControlDezoomIcon.name,
               test_api.size_button()->icon_definition_for_test()->name);
}

TEST_F(CustomFrameViewAshTest, WideFrame) {
  auto* delegate = new CustomFrameTestWidgetDelegate();
  std::unique_ptr<views::Widget> widget = CreateTestWidget(
      delegate, kShellWindowId_DefaultContainer, gfx::Rect(100, 0, 400, 500));

  CustomFrameViewAsh* custom_frame_view = delegate->custom_frame_view();
  HeaderView* header_view =
      static_cast<HeaderView*>(custom_frame_view->GetHeaderView());

  WideFrameView* wide_frame_view = WideFrameView::Create(widget.get());
  wide_frame_view->GetWidget()->Show();

  HeaderView* wide_header_view = wide_frame_view->header_view();
  display::Screen* screen = display::Screen::GetScreen();

  const gfx::Rect work_area = screen->GetPrimaryDisplay().work_area();
  gfx::Rect frame_bounds =
      wide_frame_view->GetWidget()->GetWindowBoundsInScreen();
  EXPECT_EQ(work_area.width(), frame_bounds.width());
  EXPECT_EQ(work_area.origin(), frame_bounds.origin());
  EXPECT_FALSE(header_view->should_paint());
  EXPECT_TRUE(wide_header_view->should_paint());

  Shell::Get()->window_selector_controller()->ToggleOverview();
  EXPECT_FALSE(wide_header_view->should_paint());
  Shell::Get()->window_selector_controller()->ToggleOverview();
  EXPECT_TRUE(wide_header_view->should_paint());

  // Test immersive.
  ImmersiveFullscreenController controller;
  wide_frame_view->Init(&controller);
  EXPECT_FALSE(wide_header_view->in_immersive_mode());
  EXPECT_FALSE(header_view->in_immersive_mode());

  controller.SetEnabled(ImmersiveFullscreenController::WINDOW_TYPE_OTHER, true);
  EXPECT_TRUE(header_view->in_immersive_mode());
  EXPECT_TRUE(wide_header_view->in_immersive_mode());
  // The height should be ~(33 *.5)
  wide_header_view->SetVisibleFraction(0.5);
  EXPECT_NEAR(16, wide_header_view->GetPreferredOnScreenHeight(), 1);

  // Make sure the frame can be revaled outside of the target window.
  EXPECT_FALSE(ImmersiveFullscreenControllerTestApi(&controller)
                   .IsTopEdgeHoverTimerRunning());
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.MoveMouseTo(gfx::Point(10, 0));
  generator.MoveMouseBy(1, 0);
  EXPECT_TRUE(ImmersiveFullscreenControllerTestApi(&controller)
                  .IsTopEdgeHoverTimerRunning());

  generator.MoveMouseTo(gfx::Point(10, 10));
  generator.MoveMouseBy(1, 0);
  EXPECT_FALSE(ImmersiveFullscreenControllerTestApi(&controller)
                   .IsTopEdgeHoverTimerRunning());

  generator.MoveMouseTo(gfx::Point(600, 0));
  generator.MoveMouseBy(1, 0);
  EXPECT_TRUE(ImmersiveFullscreenControllerTestApi(&controller)
                  .IsTopEdgeHoverTimerRunning());

  controller.SetEnabled(ImmersiveFullscreenController::WINDOW_TYPE_OTHER,
                        false);
  EXPECT_FALSE(header_view->in_immersive_mode());
  EXPECT_FALSE(wide_header_view->in_immersive_mode());
  // visible fraction should be ignored in non immersive.
  wide_header_view->SetVisibleFraction(0.5);
  EXPECT_EQ(32, wide_header_view->GetPreferredOnScreenHeight());

  UpdateDisplay("1234x800");
  EXPECT_EQ(1234,
            wide_frame_view->GetWidget()->GetWindowBoundsInScreen().width());
}

TEST_F(CustomFrameViewAshTest, WideFrameButton) {
  auto* delegate = new CustomFrameTestWidgetDelegate();
  std::unique_ptr<views::Widget> widget = CreateTestWidget(
      delegate, kShellWindowId_DefaultContainer, gfx::Rect(100, 0, 400, 500));

  WideFrameView* wide_frame_view = WideFrameView::Create(widget.get());
  wide_frame_view->GetWidget()->Show();
  HeaderView* header_view = wide_frame_view->header_view();
  FrameCaptionButtonContainerView::TestApi test_api(
      header_view->caption_button_container());

  EXPECT_STREQ(kWindowControlMaximizeIcon.name,
               test_api.size_button()->icon_definition_for_test()->name);
  widget->Maximize();
  header_view->Layout();
  EXPECT_STREQ(kWindowControlRestoreIcon.name,
               test_api.size_button()->icon_definition_for_test()->name);

  widget->Restore();
  header_view->Layout();
  EXPECT_STREQ(kWindowControlMaximizeIcon.name,
               test_api.size_button()->icon_definition_for_test()->name);

  widget->SetFullscreen(true);
  header_view->Layout();
  EXPECT_STREQ(kWindowControlRestoreIcon.name,
               test_api.size_button()->icon_definition_for_test()->name);

  widget->SetFullscreen(false);
  header_view->Layout();
  EXPECT_STREQ(kWindowControlMaximizeIcon.name,
               test_api.size_button()->icon_definition_for_test()->name);
}

namespace {

class CustomFrameViewAshFrameColorTest
    : public CustomFrameViewAshTest,
      public testing::WithParamInterface<bool> {
 public:
  CustomFrameViewAshFrameColorTest() = default;
  ~CustomFrameViewAshFrameColorTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(CustomFrameViewAshFrameColorTest);
};

class TestWidgetDelegate : public TestWidgetConstraintsDelegate {
 public:
  TestWidgetDelegate(bool custom) : custom_(custom) {}
  ~TestWidgetDelegate() override = default;

  // views::WidgetDelegate:
  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override {
    if (custom_) {
      ash::wm::WindowState* window_state =
          ash::wm::GetWindowState(widget->GetNativeWindow());
      window_state->SetDelegate(std::make_unique<wm::WindowStateDelegate>());
    }
    return TestWidgetConstraintsDelegate::CreateNonClientFrameView(widget);
  }

 private:
  bool custom_;

  DISALLOW_COPY_AND_ASSIGN(TestWidgetDelegate);
};

}  // namespace

// Verify that CustomFrameViewAsh updates the active color based on the
// ash::kFrameActiveColorKey window property.
TEST_P(CustomFrameViewAshFrameColorTest, kFrameActiveColorKey) {
  TestWidgetDelegate* delegate = new TestWidgetDelegate(GetParam());
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  SkColor active_color =
      widget->GetNativeWindow()->GetProperty(ash::kFrameActiveColorKey);
  constexpr SkColor new_color = SK_ColorWHITE;
  EXPECT_NE(active_color, new_color);

  widget->GetNativeWindow()->SetProperty(ash::kFrameActiveColorKey, new_color);
  active_color =
      widget->GetNativeWindow()->GetProperty(ash::kFrameActiveColorKey);
  EXPECT_EQ(active_color, new_color);
  EXPECT_EQ(new_color,
            delegate->custom_frame_view()->GetActiveFrameColorForTest());
}

// Verify that CustomFrameViewAsh updates the inactive color based on the
// ash::kFrameInactiveColorKey window property.
TEST_P(CustomFrameViewAshFrameColorTest, KFrameInactiveColor) {
  TestWidgetDelegate* delegate = new TestWidgetDelegate(GetParam());
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);

  SkColor active_color =
      widget->GetNativeWindow()->GetProperty(ash::kFrameInactiveColorKey);
  constexpr SkColor new_color = SK_ColorWHITE;
  EXPECT_NE(active_color, new_color);

  widget->GetNativeWindow()->SetProperty(ash::kFrameInactiveColorKey,
                                         new_color);
  active_color =
      widget->GetNativeWindow()->GetProperty(ash::kFrameInactiveColorKey);
  EXPECT_EQ(active_color, new_color);
  EXPECT_EQ(new_color,
            delegate->custom_frame_view()->GetInactiveFrameColorForTest());
}

// Verify that CustomFrameViewAsh updates the active color based on the
// ash::kFrameActiveColorKey window property.
TEST_P(CustomFrameViewAshFrameColorTest, WideFrameInitialColor) {
  TestWidgetDelegate* delegate = new TestWidgetDelegate(GetParam());
  std::unique_ptr<views::Widget> widget = CreateTestWidget(delegate);
  aura::Window* window = widget->GetNativeWindow();
  SkColor active_color = window->GetProperty(ash::kFrameActiveColorKey);
  SkColor inactive_color = window->GetProperty(ash::kFrameInactiveColorKey);
  constexpr SkColor new_active_color = SK_ColorWHITE;
  constexpr SkColor new_inactive_color = SK_ColorBLACK;
  EXPECT_NE(active_color, new_active_color);
  EXPECT_NE(inactive_color, new_inactive_color);
  window->SetProperty(ash::kFrameActiveColorKey, new_active_color);
  window->SetProperty(ash::kFrameInactiveColorKey, new_inactive_color);

  WideFrameView* wide_frame_view = WideFrameView::Create(widget.get());
  HeaderView* wide_header_view = wide_frame_view->header_view();
  DefaultFrameHeader* header = static_cast<DefaultFrameHeader*>(
      wide_header_view->GetFrameHeaderForTest());
  EXPECT_EQ(new_active_color, header->active_frame_color_for_testing());
  EXPECT_EQ(new_inactive_color, header->inactive_frame_color_for_testing());
}

// Run frame color tests with and without custom wm::WindowStateDelegate.
INSTANTIATE_TEST_CASE_P(, CustomFrameViewAshFrameColorTest, testing::Bool());

}  // namespace ash
