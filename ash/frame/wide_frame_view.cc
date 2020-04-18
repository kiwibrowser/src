// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/wide_frame_view.h"

#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/frame/custom_frame_view_ash.h"
#include "ash/frame/header_view.h"
#include "ash/public/cpp/ash_layout_constants.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/public/cpp/window_properties.h"
#include "ui/aura/window.h"
#include "ui/aura/window_targeter.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

class WideFrameTargeter : public aura::WindowTargeter {
 public:
  WideFrameTargeter(HeaderView* header_view) : header_view_(header_view) {}
  ~WideFrameTargeter() override = default;

  // aura::WindowTargeter:
  bool GetHitTestRects(aura::Window* target,
                       gfx::Rect* hit_test_rect_mouse,
                       gfx::Rect* hit_test_rect_touch) const override {
    if (header_view_->in_immersive_mode() && !header_view_->is_revealed()) {
      aura::Window* source = header_view_->GetWidget()->GetNativeWindow();
      *hit_test_rect_mouse = source->bounds();
      aura::Window::ConvertRectToTarget(source, target->parent(),
                                        hit_test_rect_mouse);
      hit_test_rect_mouse->set_y(target->bounds().y());
      hit_test_rect_mouse->set_height(1);
      hit_test_rect_touch->SetRect(0, 0, 0, 0);
      return true;
    }
    return aura::WindowTargeter::GetHitTestRects(target, hit_test_rect_mouse,
                                                 hit_test_rect_touch);
  }

 private:
  HeaderView* header_view_;
  DISALLOW_COPY_AND_ASSIGN(WideFrameTargeter);
};

}  // namespace

// static
WideFrameView* WideFrameView::Create(views::Widget* target) {
  auto* widget = new views::Widget;
  WideFrameView* frame_view = new WideFrameView(target, widget);

  views::Widget::InitParams params;
  params.type = views::Widget::InitParams::TYPE_POPUP;
  params.delegate = frame_view;
  params.bounds = GetFrameBounds(target);
  params.name = "WideFrameView";
  params.parent = target->GetNativeWindow();
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  widget->Init(params);

  aura::Window* window = widget->GetNativeWindow();
  window->SetEventTargeter(
      std::make_unique<WideFrameTargeter>(frame_view->header_view()));
  return frame_view;
}

// static
gfx::Rect WideFrameView::GetFrameBounds(views::Widget* target) {
  static const int kFrameHeight =
      GetAshLayoutSize(AshLayoutSize::kNonBrowserCaption).height();
  display::Screen* screen = display::Screen::GetScreen();
  aura::Window* target_window = target->GetNativeWindow();
  gfx::Rect bounds =
      target->IsFullscreen()
          ? screen->GetDisplayNearestWindow(target_window).bounds()
          : screen->GetDisplayNearestWindow(target_window).work_area();
  bounds.set_height(kFrameHeight);
  return bounds;
}

void WideFrameView::Init(ImmersiveFullscreenController* controller) {
  DCHECK(target_);
  controller->Init(this, target_, header_view_);
}

void WideFrameView::Show() {
  widget_->Show();
}

void WideFrameView::Close() {
  widget_->Close();
}

void WideFrameView::SetCaptionButtonModel(
    std::unique_ptr<CaptionButtonModel> model) {
  header_view_->caption_button_container()->SetModel(std::move(model));
  header_view_->UpdateCaptionButtons();
}

WideFrameView::WideFrameView(views::Widget* target, views::Widget* widget)
    : target_(target), widget_(widget) {
  Shell::Get()->AddShellObserver(this);
  display::Screen::GetScreen()->AddObserver(this);

  aura::Window* target_window = target->GetNativeWindow();
  target_window->AddObserver(this);
  header_view_ = new HeaderView(target);
  AddChildView(header_view_);
  GetTargetHeaderView()->SetShouldPaintHeader(false);
}

WideFrameView::~WideFrameView() {
  Shell::Get()->RemoveShellObserver(this);
  display::Screen::GetScreen()->RemoveObserver(this);
  if (target_) {
    GetTargetHeaderView()->SetShouldPaintHeader(true);
    target_->GetNativeWindow()->RemoveObserver(this);
  }
}

void WideFrameView::Layout() {
  int onscreen_height = header_view_->GetPreferredOnScreenHeight();
  if (onscreen_height == 0 || !visible()) {
    header_view_->SetVisible(false);
  } else {
    const int height = header_view_->GetPreferredHeight();
    header_view_->SetBounds(0, onscreen_height - height, width(), height);
    header_view_->SetVisible(true);
  }
}

void WideFrameView::OnWindowDestroying(aura::Window* window) {
  window->RemoveObserver(this);
  target_ = nullptr;
}

void WideFrameView::OnDisplayMetricsChanged(const display::Display& display,
                                            uint32_t changed_metrics) {
  display::Screen* screen = display::Screen::GetScreen();
  if (screen->GetDisplayNearestWindow(target_->GetNativeWindow()).id() !=
      display.id()) {
    return;
  }
  DCHECK(target_);
  GetWidget()->SetBounds(GetFrameBounds(target_));
}

void WideFrameView::OnImmersiveRevealStarted() {
  header_view_->OnImmersiveRevealStarted();
}

void WideFrameView::OnImmersiveRevealEnded() {
  header_view_->OnImmersiveRevealEnded();
}

void WideFrameView::OnImmersiveFullscreenEntered() {
  header_view_->OnImmersiveFullscreenEntered();
  if (target_)
    GetTargetHeaderView()->OnImmersiveFullscreenEntered();
}

void WideFrameView::OnImmersiveFullscreenExited() {
  header_view_->OnImmersiveFullscreenExited();
  if (target_)
    GetTargetHeaderView()->OnImmersiveFullscreenExited();
}

void WideFrameView::SetVisibleFraction(double visible_fraction) {
  header_view_->SetVisibleFraction(visible_fraction);
}

std::vector<gfx::Rect> WideFrameView::GetVisibleBoundsInScreen() const {
  return header_view_->GetVisibleBoundsInScreen();
}

void WideFrameView::OnOverviewModeStarting() {
  header_view_->SetShouldPaintHeader(false);
}
void WideFrameView::OnOverviewModeEnded() {
  header_view_->SetShouldPaintHeader(true);
}

HeaderView* WideFrameView::GetTargetHeaderView() {
  auto* frame_view = static_cast<CustomFrameViewAsh*>(
      target_->non_client_view()->frame_view());
  return static_cast<HeaderView*>(frame_view->GetHeaderView());
}

}  // namespace ash
