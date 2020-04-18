// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/immersive_mode_controller_ash.h"

#include "ash/public/cpp/immersive/immersive_revealed_lock.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/window_state_type.mojom.h"
#include "base/macros.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "chrome/browser/ui/ash/tablet_mode_client.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/mus/mus_types.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_context.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/views/background.h"
#include "ui/views/mus/mus_client.h"
#include "ui/views/view.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"
#include "ui/wm/core/window_util.h"

namespace {

// Converts from ImmersiveModeController::AnimateReveal to
// ash::ImmersiveFullscreenController::AnimateReveal.
ash::ImmersiveFullscreenController::AnimateReveal
ToImmersiveFullscreenControllerAnimateReveal(
    ImmersiveModeController::AnimateReveal animate_reveal) {
  switch (animate_reveal) {
    case ImmersiveModeController::ANIMATE_REVEAL_YES:
      return ash::ImmersiveFullscreenController::ANIMATE_REVEAL_YES;
    case ImmersiveModeController::ANIMATE_REVEAL_NO:
      return ash::ImmersiveFullscreenController::ANIMATE_REVEAL_NO;
  }
  NOTREACHED();
  return ash::ImmersiveFullscreenController::ANIMATE_REVEAL_NO;
}

class ImmersiveRevealedLockAsh : public ImmersiveRevealedLock {
 public:
  explicit ImmersiveRevealedLockAsh(ash::ImmersiveRevealedLock* lock)
      : lock_(lock) {}

 private:
  std::unique_ptr<ash::ImmersiveRevealedLock> lock_;

  DISALLOW_COPY_AND_ASSIGN(ImmersiveRevealedLockAsh);
};

// View responsible for mirroring the content of the TopContainer. This is done
// by way of mirroring the actual layers.
class TopContainerMirrorView : public views::View {
 public:
  explicit TopContainerMirrorView(views::View* view) : view_(view) {
    DCHECK(view_->layer());
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
    // At this point we have no size. Wait for the first resize before we
    // create the mirrored layer.
  }
  ~TopContainerMirrorView() override {}

  // views::View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override {
    if (mirrored_layer_tree_owner_ &&
        mirrored_layer_tree_owner_->root()->size() == size()) {
      return;
    }

    mirrored_layer_tree_owner_.reset();
    DCHECK(view_->layer());  // SetPaintToLayer() should have been called.
    mirrored_layer_tree_owner_ = wm::MirrorLayers(view_, false);
    mirrored_layer_tree_owner_->root()->SetBounds(gfx::Rect(size()));
    layer()->Add(mirrored_layer_tree_owner_->root());
  }

 private:
  views::View* view_;

  std::unique_ptr<ui::LayerTreeOwner> mirrored_layer_tree_owner_;

  DISALLOW_COPY_AND_ASSIGN(TopContainerMirrorView);
};

}  // namespace

ImmersiveModeControllerAsh::ImmersiveModeControllerAsh()
    : ImmersiveModeController(Type::ASH),
      controller_(new ash::ImmersiveFullscreenController) {}

ImmersiveModeControllerAsh::~ImmersiveModeControllerAsh() = default;

void ImmersiveModeControllerAsh::Init(BrowserView* browser_view) {
  browser_view_ = browser_view;
  controller_->Init(this, browser_view_->frame(),
      browser_view_->top_container());
}

void ImmersiveModeControllerAsh::SetEnabled(bool enabled) {
  if (controller_->IsEnabled() == enabled)
    return;

  EnableWindowObservers(enabled);

  controller_->SetEnabled(browser_view_->browser()->is_app() ?
          ash::ImmersiveFullscreenController::WINDOW_TYPE_HOSTED_APP :
          ash::ImmersiveFullscreenController::WINDOW_TYPE_BROWSER
      , enabled);
}

bool ImmersiveModeControllerAsh::IsEnabled() const {
  return controller_->IsEnabled();
}

bool ImmersiveModeControllerAsh::ShouldHideTopViews() const {
  return controller_->IsEnabled() && !controller_->IsRevealed();
}

bool ImmersiveModeControllerAsh::IsRevealed() const {
  return controller_->IsRevealed();
}

int ImmersiveModeControllerAsh::GetTopContainerVerticalOffset(
    const gfx::Size& top_container_size) const {
  if (!IsEnabled())
    return 0;

  return static_cast<int>(top_container_size.height() *
                          (visible_fraction_ - 1));
}

ImmersiveRevealedLock* ImmersiveModeControllerAsh::GetRevealedLock(
    AnimateReveal animate_reveal) {
  return new ImmersiveRevealedLockAsh(controller_->GetRevealedLock(
      ToImmersiveFullscreenControllerAnimateReveal(animate_reveal)));
}

void ImmersiveModeControllerAsh::OnFindBarVisibleBoundsChanged(
    const gfx::Rect& new_visible_bounds_in_screen) {
  find_bar_visible_bounds_in_screen_ = new_visible_bounds_in_screen;
}

bool ImmersiveModeControllerAsh::ShouldStayImmersiveAfterExitingFullscreen() {
  // TODO(crbug.com/760811): Support tablet mode in mash.
  if (ash_util::IsRunningInMash())
    return false;

  return !browser_view_->IsBrowserTypeNormal() &&
         TabletModeClient::Get()->tablet_mode_enabled();
}

views::Widget* ImmersiveModeControllerAsh::GetRevealWidget() {
  return mash_reveal_widget_.get();
}

void ImmersiveModeControllerAsh::OnWidgetActivationChanged(
    views::Widget* widget,
    bool active) {
  if (browser_view_->IsBrowserTypeNormal())
    return;

  // TODO(crbug.com/760811): Support tablet mode in mash.
  if (ash_util::IsRunningInMash() ||
      !TabletModeClient::Get()->tablet_mode_enabled()) {
    return;
  }

  // Enable immersive mode if the widget is activated. Do not disable immersive
  // mode if the widget deactivates, but is not minimized.
  controller_->SetEnabled(
      browser_view_->browser()->is_app()
          ? ash::ImmersiveFullscreenController::WINDOW_TYPE_HOSTED_APP
          : ash::ImmersiveFullscreenController::WINDOW_TYPE_BROWSER,
      active || !widget->IsMinimized());
}

void ImmersiveModeControllerAsh::EnableWindowObservers(bool enable) {
  if (observers_enabled_ == enable)
    return;
  observers_enabled_ = enable;

  aura::Window* native_window = browser_view_->GetNativeWindow();
  aura::Window* target_window = ash_util::IsRunningInMash()
                                    ? native_window->GetRootWindow()
                                    : native_window;

  content::Source<FullscreenController> source(browser_view_->browser()
                                                   ->exclusive_access_manager()
                                                   ->fullscreen_controller());
  if (enable) {
    target_window->AddObserver(this);
    registrar_.Add(this, chrome::NOTIFICATION_FULLSCREEN_CHANGED, source);
  } else {
    target_window->RemoveObserver(this);
    registrar_.Remove(this, chrome::NOTIFICATION_FULLSCREEN_CHANGED, source);
  }
}

void ImmersiveModeControllerAsh::LayoutBrowserRootView() {
  views::Widget* widget = browser_view_->frame();
  // Update the window caption buttons.
  widget->non_client_view()->frame_view()->ResetWindowControls();
  widget->non_client_view()->frame_view()->InvalidateLayout();
  browser_view_->InvalidateLayout();
  widget->GetRootView()->Layout();
}

void ImmersiveModeControllerAsh::CreateMashRevealWidget() {
  if (!ash_util::IsRunningInMash())
    return;

  DCHECK(!mash_reveal_widget_);
  mash_reveal_widget_ = std::make_unique<views::Widget>();
  views::Widget::InitParams init_params(views::Widget::InitParams::TYPE_POPUP);
  init_params.mus_properties
      [ui::mojom::WindowManager::kRenderParentTitleArea_Property] =
      mojo::ConvertTo<std::vector<uint8_t>>(
          static_cast<aura::PropertyConverter::PrimitiveType>(true));
  init_params.mus_properties
      [ui::mojom::WindowManager::kWindowIgnoredByShelf_InitProperty] =
      mojo::ConvertTo<std::vector<uint8_t>>(true);
  init_params.name = "ChromeImmersiveRevealWindow";
  // We want events to fall through to the real views.
  init_params.accept_events = false;
  init_params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  init_params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  init_params.parent = browser_view_->GetNativeWindow()->GetRootWindow();
  // The widget needs to be translucent so the frame decorations drawn by the
  // window manager are visible.
  init_params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  const gfx::Rect& top_container_bounds =
      browser_view_->top_container()->bounds();
  init_params.bounds =
      gfx::Rect(0, -top_container_bounds.height(), top_container_bounds.width(),
                top_container_bounds.height());
  mash_reveal_widget_->Init(init_params);
  mash_reveal_widget_->SetContentsView(
      new TopContainerMirrorView(browser_view_->top_container()));
  mash_reveal_widget_->StackAtTop();
  mash_reveal_widget_->Show();
}

void ImmersiveModeControllerAsh::DestroyMashRevealWidget() {
  mash_reveal_widget_.reset();
}

void ImmersiveModeControllerAsh::OnImmersiveRevealStarted() {
  DestroyMashRevealWidget();

  visible_fraction_ = 0;
  browser_view_->top_container()->SetPaintToLayer();
  browser_view_->top_container()->layer()->SetFillsBoundsOpaquely(false);
  LayoutBrowserRootView();
  CreateMashRevealWidget();
  for (Observer& observer : observers_)
    observer.OnImmersiveRevealStarted();
}

void ImmersiveModeControllerAsh::OnImmersiveRevealEnded() {
  DestroyMashRevealWidget();
  visible_fraction_ = 0;
  browser_view_->top_container()->DestroyLayer();
  LayoutBrowserRootView();
  for (Observer& observer : observers_)
    observer.OnImmersiveRevealEnded();
}

void ImmersiveModeControllerAsh::OnImmersiveFullscreenEntered() {}

void ImmersiveModeControllerAsh::OnImmersiveFullscreenExited() {
  DestroyMashRevealWidget();
  browser_view_->top_container()->DestroyLayer();
  LayoutBrowserRootView();
  for (Observer& observer : observers_)
    observer.OnImmersiveFullscreenExited();
}

void ImmersiveModeControllerAsh::SetVisibleFraction(double visible_fraction) {
  if (visible_fraction_ == visible_fraction)
    return;

  visible_fraction_ = visible_fraction;
  browser_view_->Layout();
  browser_view_->frame()->GetFrameView()->UpdateClientArea();

  if (mash_reveal_widget_) {
    gfx::Rect bounds = mash_reveal_widget_->GetNativeWindow()->bounds();
    bounds.set_y(visible_fraction * bounds.height() - bounds.height());
    mash_reveal_widget_->SetBounds(bounds);
  }
}

std::vector<gfx::Rect>
ImmersiveModeControllerAsh::GetVisibleBoundsInScreen() const {
  views::View* top_container_view = browser_view_->top_container();
  gfx::Rect top_container_view_bounds = top_container_view->GetVisibleBounds();
  // TODO(tdanderson): Implement View::ConvertRectToScreen().
  gfx::Point top_container_view_bounds_in_screen_origin(
      top_container_view_bounds.origin());
  views::View::ConvertPointToScreen(top_container_view,
      &top_container_view_bounds_in_screen_origin);
  gfx::Rect top_container_view_bounds_in_screen(
      top_container_view_bounds_in_screen_origin,
      top_container_view_bounds.size());

  std::vector<gfx::Rect> bounds_in_screen;
  bounds_in_screen.push_back(top_container_view_bounds_in_screen);
  bounds_in_screen.push_back(find_bar_visible_bounds_in_screen_);
  return bounds_in_screen;
}

void ImmersiveModeControllerAsh::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_FULLSCREEN_CHANGED, type);
  if (!controller_->IsEnabled())
    return;

  // Auto hide the shelf in immersive browser fullscreen.
  bool in_tab_fullscreen = content::Source<FullscreenController>(source)->
      IsWindowFullscreenForTabOrPending();
  browser_view_->GetNativeWindow()->SetProperty(
      ash::kHideShelfWhenFullscreenKey, in_tab_fullscreen);
}

void ImmersiveModeControllerAsh::OnWindowPropertyChanged(aura::Window* window,
                                                         const void* key,
                                                         intptr_t old) {
  if (key == ash::kWindowStateTypeKey) {
    ash::mojom::WindowStateType new_state =
        window->GetProperty(ash::kWindowStateTypeKey);
    ash::mojom::WindowStateType old_state = ash::mojom::WindowStateType(old);

    // Disable immersive fullscreen when the user exits fullscreen without going
    // through FullscreenController::ToggleBrowserFullscreenMode(). This is the
    // case if the user exits fullscreen via the restore button.
    if (controller_->IsEnabled() &&
        new_state != ash::mojom::WindowStateType::FULLSCREEN &&
        new_state != ash::mojom::WindowStateType::PINNED &&
        new_state != ash::mojom::WindowStateType::TRUSTED_PINNED &&
        new_state != ash::mojom::WindowStateType::MINIMIZED &&
        old_state == ash::mojom::WindowStateType::FULLSCREEN) {
      browser_view_->FullscreenStateChanged();
    }
  }
}

void ImmersiveModeControllerAsh::OnWindowDestroying(aura::Window* window) {
  // Clean up observers here rather than in the destructor because the owning
  // BrowserView has already destroyed the aura::Window.
  EnableWindowObservers(false);
}
