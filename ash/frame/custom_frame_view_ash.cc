// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/custom_frame_view_ash.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "ash/frame/caption_buttons/frame_caption_button.h"
#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/frame/default_frame_header.h"
#include "ash/frame/frame_border_hit_test.h"
#include "ash/frame/header_view.h"
#include "ash/public/cpp/ash_constants.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller_delegate.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/shell.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "ash/wm/resize_handle_window_targeter.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_observer.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_delegate.h"
#include "ash/wm/window_state_observer.h"
#include "ash/wm/window_util.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/view.h"
#include "ui/views/view_targeter.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

namespace {

///////////////////////////////////////////////////////////////////////////////
// CustomFrameViewAshWindowStateDelegate

// Handles a user's fullscreen request (Shift+F4/F4). Puts the window into
// immersive fullscreen if immersive fullscreen is enabled for non-browser
// windows.
class CustomFrameViewAshWindowStateDelegate : public wm::WindowStateDelegate,
                                              public wm::WindowStateObserver,
                                              public aura::WindowObserver,
                                              public TabletModeObserver {
 public:
  CustomFrameViewAshWindowStateDelegate(wm::WindowState* window_state,
                                        CustomFrameViewAsh* custom_frame_view,
                                        bool enable_immersive)
      : window_state_(nullptr) {
    // Add a window state observer to exit fullscreen properly in case
    // fullscreen is exited without going through
    // WindowState::ToggleFullscreen(). This is the case when exiting
    // immersive fullscreen via the "Restore" window control.
    // TODO(pkotwicz): This is a hack. Remove ASAP. http://crbug.com/319048
    window_state_ = window_state;
    window_state_->AddObserver(this);
    window_state_->window()->AddObserver(this);

    if (!enable_immersive)
      return;

    Shell::Get()->tablet_mode_controller()->AddObserver(this);

    immersive_fullscreen_controller_ =
        std::make_unique<ImmersiveFullscreenController>();
    custom_frame_view->InitImmersiveFullscreenControllerForView(
        immersive_fullscreen_controller_.get());
  }

  ~CustomFrameViewAshWindowStateDelegate() override {
    if (Shell::Get()->tablet_mode_controller())
      Shell::Get()->tablet_mode_controller()->RemoveObserver(this);

    if (window_state_) {
      window_state_->RemoveObserver(this);
      window_state_->window()->RemoveObserver(this);
    }
  }

  // TabletModeObserver:
  void OnTabletModeStarted() override {
    if (window_state_->IsFullscreen())
      return;
    views::Widget* widget =
        views::Widget::GetWidgetForNativeWindow(window_state_->window());
    if (Shell::Get()->tablet_mode_controller()->ShouldAutoHideTitlebars(
            widget)) {
      immersive_fullscreen_controller_->SetEnabled(
          ImmersiveFullscreenController::WINDOW_TYPE_OTHER, true);
    }
  }

  void OnTabletModeEnded() override {
    if (window_state_->IsFullscreen())
      return;

    immersive_fullscreen_controller_->SetEnabled(
        ImmersiveFullscreenController::WINDOW_TYPE_OTHER, false);
  }

 private:
  // wm::WindowStateDelegate:
  bool ToggleFullscreen(wm::WindowState* window_state) override {
    bool enter_fullscreen = !window_state->IsFullscreen();
    if (enter_fullscreen) {
      window_state_->window()->SetProperty(aura::client::kShowStateKey,
                                           ui::SHOW_STATE_FULLSCREEN);
    } else {
      window_state->Restore();
    }
    if (immersive_fullscreen_controller_) {
      immersive_fullscreen_controller_->SetEnabled(
          ImmersiveFullscreenController::WINDOW_TYPE_OTHER, enter_fullscreen);
    }
    return true;
  }

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override {
    window_state_->RemoveObserver(this);
    window->RemoveObserver(this);
    window_state_ = nullptr;
  }

  // wm::WindowStateObserver:
  void OnPostWindowStateTypeChange(wm::WindowState* window_state,
                                   mojom::WindowStateType old_type) override {
    views::Widget* widget =
        views::Widget::GetWidgetForNativeWindow(window_state->window());
    if (Shell::Get()->tablet_mode_controller() &&
        Shell::Get()->tablet_mode_controller()->ShouldAutoHideTitlebars(
            widget)) {
      DCHECK(immersive_fullscreen_controller_);
      if (window_state->IsMinimized() &&
          immersive_fullscreen_controller_->IsEnabled()) {
        immersive_fullscreen_controller_->SetEnabled(
            ImmersiveFullscreenController::WINDOW_TYPE_OTHER, false);
      } else if (window_state->IsMaximized() &&
                 !immersive_fullscreen_controller_->IsEnabled()) {
        immersive_fullscreen_controller_->SetEnabled(
            ImmersiveFullscreenController::WINDOW_TYPE_OTHER, true);
      }
      return;
    }

    if (!window_state->IsFullscreen() && !window_state->IsMinimized() &&
        immersive_fullscreen_controller_ &&
        immersive_fullscreen_controller_->IsEnabled()) {
      immersive_fullscreen_controller_->SetEnabled(
          ImmersiveFullscreenController::WINDOW_TYPE_OTHER, false);
    }
  }

  CustomFrameViewAsh* GetFrameView() {
    views::Widget* widget =
        views::Widget::GetWidgetForNativeWindow(window_state_->window());
    return static_cast<CustomFrameViewAsh*>(
        widget->non_client_view()->frame_view());
  }

  wm::WindowState* window_state_;
  std::unique_ptr<ImmersiveFullscreenController>
      immersive_fullscreen_controller_;

  DISALLOW_COPY_AND_ASSIGN(CustomFrameViewAshWindowStateDelegate);
};

}  // namespace

// static
bool CustomFrameViewAsh::use_empty_minimum_size_for_test_ = false;

///////////////////////////////////////////////////////////////////////////////
// CustomFrameViewAsh::OverlayView

// View which takes up the entire widget and contains the HeaderView. HeaderView
// is a child of OverlayView to avoid creating a larger texture than necessary
// when painting the HeaderView to its own layer.
class CustomFrameViewAsh::OverlayView : public views::View,
                                        public views::ViewTargeterDelegate {
 public:
  explicit OverlayView(HeaderView* header_view);
  ~OverlayView() override;

  void SetHeaderHeight(base::Optional<int> height);

  // views::View:
  void Layout() override;
  const char* GetClassName() const override { return "OverlayView"; }

 private:
  // views::ViewTargeterDelegate:
  bool DoesIntersectRect(const views::View* target,
                         const gfx::Rect& rect) const override;

  HeaderView* header_view_;

  base::Optional<int> header_height_;

  DISALLOW_COPY_AND_ASSIGN(OverlayView);
};

CustomFrameViewAsh::OverlayView::OverlayView(HeaderView* header_view)
    : header_view_(header_view) {
  AddChildView(header_view);
  SetEventTargeter(
      std::unique_ptr<views::ViewTargeter>(new views::ViewTargeter(this)));
}

CustomFrameViewAsh::OverlayView::~OverlayView() = default;

void CustomFrameViewAsh::OverlayView::SetHeaderHeight(
    base::Optional<int> height) {
  if (header_height_ == height)
    return;

  header_height_ = height;
  Layout();
}

///////////////////////////////////////////////////////////////////////////////
// CustomFrameViewAsh::OverlayView, views::View overrides:

void CustomFrameViewAsh::OverlayView::Layout() {
  // Layout |header_view_| because layout affects the result of
  // GetPreferredOnScreenHeight().
  header_view_->Layout();

  int onscreen_height = header_height_
                            ? *header_height_
                            : header_view_->GetPreferredOnScreenHeight();
  if (onscreen_height == 0 || !visible()) {
    header_view_->SetVisible(false);
  } else {
    const int height =
        header_height_ ? *header_height_ : header_view_->GetPreferredHeight();
    header_view_->SetBounds(0, onscreen_height - height, width(), height);
    header_view_->SetVisible(true);
  }
}

///////////////////////////////////////////////////////////////////////////////
// CustomFrameViewAsh::OverlayView, views::ViewTargeterDelegate overrides:

bool CustomFrameViewAsh::OverlayView::DoesIntersectRect(
    const views::View* target,
    const gfx::Rect& rect) const {
  CHECK_EQ(target, this);
  // Grab events in the header view. Return false for other events so that they
  // can be handled by the client view.
  return header_view_->HitTestRect(rect);
}

////////////////////////////////////////////////////////////////////////////////
// CustomFrameViewAsh, public:

// static
const char CustomFrameViewAsh::kViewClassName[] = "CustomFrameViewAsh";

CustomFrameViewAsh::CustomFrameViewAsh(
    views::Widget* frame,
    ImmersiveFullscreenControllerDelegate* immersive_delegate,
    bool enable_immersive,
    mojom::WindowStyle window_style,
    std::unique_ptr<CaptionButtonModel> model)
    : frame_(frame),
      header_view_(new HeaderView(frame, window_style, std::move(model))),
      overlay_view_(new OverlayView(header_view_)),
      immersive_delegate_(immersive_delegate ? immersive_delegate
                                             : header_view_) {
  aura::Window* frame_window = frame->GetNativeWindow();
  wm::InstallResizeHandleWindowTargeterForWindow(frame_window, nullptr);
  // |header_view_| is set as the non client view's overlay view so that it can
  // overlay the web contents in immersive fullscreen.
  frame->non_client_view()->SetOverlayView(overlay_view_);
  frame_window->SetProperty(aura::client::kTopViewColor, kDefaultFrameColor);
  frame_window->AddObserver(this);

  // A delegate for a more complex way of fullscreening the window may already
  // be set. This is the case for packaged apps.
  wm::WindowState* window_state = wm::GetWindowState(frame_window);
  if (!window_state->HasDelegate()) {
    window_state->SetDelegate(std::unique_ptr<wm::WindowStateDelegate>(
        new CustomFrameViewAshWindowStateDelegate(window_state, this,
                                                  enable_immersive)));
  }
  Shell::Get()->AddShellObserver(this);
  Shell::Get()->split_view_controller()->AddObserver(this);
}

CustomFrameViewAsh::~CustomFrameViewAsh() {
  Shell::Get()->RemoveShellObserver(this);
  if (Shell::Get()->split_view_controller())
    Shell::Get()->split_view_controller()->RemoveObserver(this);
  if (frame_ && frame_->GetNativeWindow() &&
      frame_->GetNativeWindow()->HasObserver(this)) {
    frame_->GetNativeWindow()->RemoveObserver(this);
  }
}

void CustomFrameViewAsh::InitImmersiveFullscreenControllerForView(
    ImmersiveFullscreenController* immersive_fullscreen_controller) {
  immersive_fullscreen_controller->Init(immersive_delegate_, frame_,
                                        header_view_);
}

void CustomFrameViewAsh::SetFrameColors(SkColor active_frame_color,
                                        SkColor inactive_frame_color) {
  aura::Window* frame_window = frame_->GetNativeWindow();
  frame_window->SetProperty(aura::client::kTopViewColor, inactive_frame_color);
  frame_window->SetProperty(ash::kFrameActiveColorKey, active_frame_color);
  frame_window->SetProperty(ash::kFrameInactiveColorKey, inactive_frame_color);
}

void CustomFrameViewAsh::SetCaptionButtonModel(
    std::unique_ptr<CaptionButtonModel> model) {
  header_view_->caption_button_container()->SetModel(std::move(model));
  header_view_->UpdateCaptionButtons();
}

void CustomFrameViewAsh::SetHeaderHeight(base::Optional<int> height) {
  overlay_view_->SetHeaderHeight(height);
}

views::View* CustomFrameViewAsh::GetHeaderView() {
  return header_view_;
}

gfx::Rect CustomFrameViewAsh::GetClientBoundsForWindowBounds(
    const gfx::Rect& window_bounds) const {
  gfx::Rect client_bounds(window_bounds);
  client_bounds.Inset(0, NonClientTopBorderHeight(), 0, 0);
  return client_bounds;
}

////////////////////////////////////////////////////////////////////////////////
// CustomFrameViewAsh, views::NonClientFrameView overrides:

gfx::Rect CustomFrameViewAsh::GetBoundsForClientView() const {
  gfx::Rect client_bounds = bounds();
  client_bounds.Inset(0, NonClientTopBorderHeight(), 0, 0);
  return client_bounds;
}

gfx::Rect CustomFrameViewAsh::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  gfx::Rect window_bounds = client_bounds;
  window_bounds.Inset(0, -NonClientTopBorderHeight(), 0, 0);
  return window_bounds;
}

int CustomFrameViewAsh::NonClientHitTest(const gfx::Point& point) {
  return FrameBorderNonClientHitTest(this, header_view_->GetBackButton(),
                                     header_view_->caption_button_container(),
                                     point);
}

void CustomFrameViewAsh::GetWindowMask(const gfx::Size& size,
                                       gfx::Path* window_mask) {
  // No window masks in Aura.
}

void CustomFrameViewAsh::ResetWindowControls() {
  header_view_->ResetWindowControls();
}

void CustomFrameViewAsh::UpdateWindowIcon() {}

void CustomFrameViewAsh::UpdateWindowTitle() {
  header_view_->SchedulePaintForTitle();
}

void CustomFrameViewAsh::SizeConstraintsChanged() {
  header_view_->UpdateCaptionButtons();
}

void CustomFrameViewAsh::ActivationChanged(bool active) {
  // The icons differ between active and inactive.
  header_view_->SchedulePaint();
  frame_->non_client_view()->Layout();
}

////////////////////////////////////////////////////////////////////////////////
// CustomFrameViewAsh, views::View overrides:

gfx::Size CustomFrameViewAsh::CalculatePreferredSize() const {
  gfx::Size pref = frame_->client_view()->GetPreferredSize();
  gfx::Rect bounds(0, 0, pref.width(), pref.height());
  return frame_->non_client_view()
      ->GetWindowBoundsForClientBounds(bounds)
      .size();
}

void CustomFrameViewAsh::Layout() {
  if (!enabled())
    return;
  views::NonClientFrameView::Layout();
  aura::Window* frame_window = frame_->GetNativeWindow();
  frame_window->SetProperty(aura::client::kTopViewInset,
                            NonClientTopBorderHeight());
}

const char* CustomFrameViewAsh::GetClassName() const {
  return kViewClassName;
}

gfx::Size CustomFrameViewAsh::GetMinimumSize() const {
  if (use_empty_minimum_size_for_test_ || !enabled())
    return gfx::Size();

  gfx::Size min_client_view_size(frame_->client_view()->GetMinimumSize());
  gfx::Size min_size(
      std::max(header_view_->GetMinimumWidth(), min_client_view_size.width()),
      NonClientTopBorderHeight() + min_client_view_size.height());

  aura::Window* frame_window = frame_->GetNativeWindow();
  const gfx::Size* min_window_size =
      frame_window->GetProperty(aura::client::kMinimumSize);
  if (min_window_size)
    min_size.SetToMax(*min_window_size);
  return min_size;
}

gfx::Size CustomFrameViewAsh::GetMaximumSize() const {
  gfx::Size max_client_size(frame_->client_view()->GetMaximumSize());
  int width = 0;
  int height = 0;

  if (max_client_size.width() > 0)
    width = std::max(header_view_->GetMinimumWidth(), max_client_size.width());
  if (max_client_size.height() > 0)
    height = NonClientTopBorderHeight() + max_client_size.height();

  return gfx::Size(width, height);
}

void CustomFrameViewAsh::SchedulePaintInRect(const gfx::Rect& r) {
  // We may end up here before |header_view_| has been added to the Widget.
  if (header_view_->GetWidget()) {
    // The HeaderView is not a child of CustomFrameViewAsh. Redirect the paint
    // to HeaderView instead.
    gfx::RectF to_paint(r);
    views::View::ConvertRectToTarget(this, header_view_, &to_paint);
    header_view_->SchedulePaintInRect(gfx::ToEnclosingRect(to_paint));
  } else {
    views::NonClientFrameView::SchedulePaintInRect(r);
  }
}

void CustomFrameViewAsh::SetVisible(bool visible) {
  views::View::SetVisible(visible);
  // We need to re-layout so that client view will occupy entire window.
  InvalidateLayout();
}

void CustomFrameViewAsh::OnWindowDestroying(aura::Window* window) {
  DCHECK_EQ(frame_->GetNativeWindow(), window);
  window->RemoveObserver(this);
}

void CustomFrameViewAsh::OnWindowPropertyChanged(aura::Window* window,
                                                 const void* key,
                                                 intptr_t old) {
  DCHECK_EQ(frame_->GetNativeWindow(), window);
  if (key == aura::client::kShowStateKey) {
    header_view_->OnShowStateChanged(
        window->GetProperty(aura::client::kShowStateKey));
  }
}

const views::View* CustomFrameViewAsh::GetAvatarIconViewForTest() const {
  return header_view_->avatar_icon();
}

SkColor CustomFrameViewAsh::GetActiveFrameColorForTest() const {
  return frame_->GetNativeWindow()->GetProperty(ash::kFrameActiveColorKey);
}

SkColor CustomFrameViewAsh::GetInactiveFrameColorForTest() const {
  return frame_->GetNativeWindow()->GetProperty(ash::kFrameInactiveColorKey);
}

void CustomFrameViewAsh::UpdateHeaderView() {
  SplitViewController* split_view_controller =
      Shell::Get()->split_view_controller();
  if (in_overview_mode_ && split_view_controller->IsSplitViewModeActive() &&
      split_view_controller->GetDefaultSnappedWindow() ==
          frame_->GetNativeWindow()) {
    // TODO(sammiequon): This works for now, but we may have to check if
    // |frame_|'s native window is in the overview list instead.
    SetShouldPaintHeader(true);
  } else {
    SetShouldPaintHeader(!in_overview_mode_);
  }
}

void CustomFrameViewAsh::SetShouldPaintHeader(bool paint) {
  header_view_->SetShouldPaintHeader(paint);
}

void CustomFrameViewAsh::OnOverviewModeStarting() {
  in_overview_mode_ = true;
  UpdateHeaderView();
}

void CustomFrameViewAsh::OnOverviewModeEnded() {
  in_overview_mode_ = false;
  UpdateHeaderView();
}

void CustomFrameViewAsh::OnSplitViewStateChanged(
    SplitViewController::State /* previous_state */,
    SplitViewController::State /* current_state */) {
  UpdateHeaderView();
}

////////////////////////////////////////////////////////////////////////////////
// CustomFrameViewAsh, private:

// views::NonClientFrameView:
bool CustomFrameViewAsh::DoesIntersectRect(const views::View* target,
                                           const gfx::Rect& rect) const {
  CHECK_EQ(target, this);
  // NonClientView hit tests the NonClientFrameView first instead of going in
  // z-order. Return false so that events get to the OverlayView.
  return false;
}

FrameCaptionButtonContainerView*
CustomFrameViewAsh::GetFrameCaptionButtonContainerViewForTest() {
  return header_view_->caption_button_container();
}

int CustomFrameViewAsh::NonClientTopBorderHeight() const {
  // The frame should not occupy the window area when it's in fullscreen,
  // not visible or disabled.
  if (frame_->IsFullscreen() || !visible() || !enabled() ||
      header_view_->in_immersive_mode()) {
    return 0;
  }
  return header_view_->GetPreferredHeight();
}

}  // namespace ash
