// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view_ash.h"

#include <algorithm>

#include "ash/frame/caption_buttons/frame_back_button.h"
#include "ash/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/frame/default_frame_header.h"
#include "ash/frame/frame_border_hit_test.h"
#include "ash/frame/frame_header_origin_text.h"
#include "ash/frame/frame_header_util.h"
#include "ash/public/cpp/app_types.h"
#include "ash/public/cpp/ash_constants.h"
#include "ash/public/cpp/ash_layout_constants.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/public/interfaces/window_state_type.mojom.h"
#include "ash/shell.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "ash/wm/window_util.h"
#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "chrome/browser/ui/ash/tablet_mode_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/extensions/hosted_app_browser_controller.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#include "chrome/browser/ui/views/frame/browser_frame_ash.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/hosted_app_button_container.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/views/profiles/profile_indicator_icon.h"
#include "chrome/browser/ui/views/tab_icon_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/browser/web_applications/web_app.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/base/layout.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/label.h"
#include "ui/views/rect_based_targeting_utils.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {

bool IsV1AppBackButtonEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      ash::switches::kAshEnableV1AppBackButton);
}

// Returns true if |window| is currently snapped in split view mode.
bool IsSnappedInSplitView(aura::Window* window,
                          ash::mojom::SplitViewState state) {
  ash::mojom::WindowStateType type =
      window->GetProperty(ash::kWindowStateTypeKey);
  switch (state) {
    case ash::mojom::SplitViewState::NO_SNAP:
      return false;
    case ash::mojom::SplitViewState::LEFT_SNAPPED:
      return type == ash::mojom::WindowStateType::LEFT_SNAPPED;
    case ash::mojom::SplitViewState::RIGHT_SNAPPED:
      return type == ash::mojom::WindowStateType::RIGHT_SNAPPED;
    case ash::mojom::SplitViewState::BOTH_SNAPPED:
      return type == ash::mojom::WindowStateType::LEFT_SNAPPED ||
             type == ash::mojom::WindowStateType::RIGHT_SNAPPED;
    default:
      NOTREACHED();
      return false;
  }
}

void SetRightSide(gfx::Rect* rect, int x) {
  rect->set_x(x - rect->width());
  DCHECK_EQ(rect->right(), x);
}

void AlignVerticalCenterWith(gfx::Rect* rect, const gfx::Rect& sibling) {
  rect->set_y(sibling.CenterPoint().y() - rect->height() / 2);
  DCHECK_EQ(rect->CenterPoint().y(), sibling.CenterPoint().y());
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// BrowserNonClientFrameViewAsh, public:

const base::TimeDelta BrowserNonClientFrameViewAsh::kTitlebarAnimationDelay =
    base::TimeDelta::FromMilliseconds(750);

BrowserNonClientFrameViewAsh::BrowserNonClientFrameViewAsh(
    BrowserFrame* frame,
    BrowserView* browser_view)
    : BrowserNonClientFrameView(frame, browser_view),
      caption_button_container_(nullptr),
      back_button_(nullptr),
      window_icon_(nullptr),
      hosted_app_button_container_(nullptr),
      observer_binding_(this),
      weak_factory_(this) {
  ash::wm::InstallResizeHandleWindowTargeterForWindow(frame->GetNativeWindow(),
                                                      nullptr);
  ash::Shell::Get()->AddShellObserver(this);

  // The ServiceManagerConnection may be nullptr in tests.
  if (content::ServiceManagerConnection::GetForProcess()) {
    content::ServiceManagerConnection::GetForProcess()
        ->GetConnector()
        ->BindInterface(ash::mojom::kServiceName, &split_view_controller_);
    ash::mojom::SplitViewObserverPtr observer;
    observer_binding_.Bind(mojo::MakeRequest(&observer));
    split_view_controller_->AddObserver(std::move(observer));
  }
}

BrowserNonClientFrameViewAsh::~BrowserNonClientFrameViewAsh() {
  ImmersiveModeController* immersive_controller =
      browser_view()->immersive_mode_controller();
  if (immersive_controller)
    immersive_controller->RemoveObserver(this);

  if (frame() && frame()->GetNativeWindow() &&
      frame()->GetNativeWindow()->HasObserver(this)) {
    frame()->GetNativeWindow()->RemoveObserver(this);
  }
  if (TabletModeClient::Get())
    TabletModeClient::Get()->RemoveObserver(this);
  ash::Shell::Get()->RemoveShellObserver(this);
  if (back_button_) {
    browser_view()->browser()->command_controller()->RemoveCommandObserver(
        this);
  }
}

void BrowserNonClientFrameViewAsh::Init() {
  caption_button_container_ = new ash::FrameCaptionButtonContainerView(frame());
  caption_button_container_->UpdateCaptionButtonState(false /*=animate*/);
  AddChildView(caption_button_container_);

  Browser* browser = browser_view()->browser();

  // Initializing the TabIconView is expensive, so only do it if we need to.
  if (browser_view()->ShouldShowWindowIcon()) {
    window_icon_ = new TabIconView(this, nullptr);
    window_icon_->set_is_light(true);
    AddChildView(window_icon_);
    window_icon_->Update();
  }
  if (browser->is_app() && IsV1AppBackButtonEnabled()) {
    back_button_ = new ash::FrameBackButton();
    AddChildView(back_button_);
    // TODO(oshima): Add Tooltip, accessibility name.
    browser->command_controller()->AddCommandObserver(IDC_BACK, this);
  }

  frame_header_ = CreateFrameHeader();

  if (browser->is_app()) {
    frame()->GetNativeWindow()->SetProperty(
        aura::client::kAppType, static_cast<int>(ash::AppType::CHROME_APP));
  } else {
    frame()->GetNativeWindow()->SetProperty(
        aura::client::kAppType, static_cast<int>(ash::AppType::BROWSER));
  }

  // TabletModeClient may not be initialized during unit tests.
  if (TabletModeClient::Get())
    TabletModeClient::Get()->AddObserver(this);

  frame()->GetNativeWindow()->AddObserver(this);

  browser_view()->immersive_mode_controller()->AddObserver(this);
}

ash::mojom::SplitViewObserverPtr
BrowserNonClientFrameViewAsh::CreateInterfacePtrForTesting() {
  if (observer_binding_.is_bound())
    observer_binding_.Unbind();
  ash::mojom::SplitViewObserverPtr ptr;
  observer_binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

///////////////////////////////////////////////////////////////////////////////
// BrowserNonClientFrameView:

gfx::Rect BrowserNonClientFrameViewAsh::GetBoundsForTabStrip(
    views::View* tabstrip) const {
  if (!tabstrip)
    return gfx::Rect();

  const int left_inset = GetTabStripLeftInset();
  return gfx::Rect(left_inset, GetTopInset(false),
                   std::max(0, width() - left_inset - GetTabStripRightInset()),
                   tabstrip->GetPreferredSize().height());
}

int BrowserNonClientFrameViewAsh::GetTopInset(bool restored) const {
  if (!ShouldPaint()) {
    // When immersive fullscreen unrevealed, tabstrip is offscreen with normal
    // tapstrip bounds, the top inset should reach this topmost edge.
    const ImmersiveModeController* const immersive_controller =
        browser_view()->immersive_mode_controller();
    if (immersive_controller->IsEnabled() &&
        !immersive_controller->IsRevealed()) {
      return (-1) * browser_view()->GetTabStripHeight();
    }

    // The header isn't painted for restored popup/app windows in overview mode,
    // but the inset is still calculated below, so the overview code can align
    // the window content with a fake header.
    if (!in_overview_mode_ || frame()->IsFullscreen() ||
        browser_view()->IsTabStripVisible()) {
      return 0;
    }
  }

  if (!browser_view()->IsTabStripVisible()) {
    return (UsePackagedAppHeaderStyle())
               ? frame_header_->GetHeaderHeight()
               : caption_button_container_->bounds().bottom();
  }

  const int header_height =
      restored ? GetAshLayoutSize(ash::AshLayoutSize::kBrowserCaptionRestored)
                     .height()
               : frame_header_->GetHeaderHeight();
  return header_height - browser_view()->GetTabStripHeight();
}

int BrowserNonClientFrameViewAsh::GetThemeBackgroundXInset() const {
  return ash::FrameHeaderUtil::GetThemeBackgroundXInset();
}

void BrowserNonClientFrameViewAsh::UpdateThrobber(bool running) {
  if (window_icon_)
    window_icon_->Update();
}

void BrowserNonClientFrameViewAsh::UpdateMinimumSize() {
  gfx::Size min_size = GetMinimumSize();
  aura::Window* frame_window = frame()->GetNativeWindow();
  const gfx::Size* previous_min_size =
      frame_window->GetProperty(aura::client::kMinimumSize);
  if (!previous_min_size || *previous_min_size != min_size) {
    frame_window->SetProperty(aura::client::kMinimumSize,
                              new gfx::Size(min_size));
  }
}

///////////////////////////////////////////////////////////////////////////////
// views::NonClientFrameView:

gfx::Rect BrowserNonClientFrameViewAsh::GetBoundsForClientView() const {
  // The ClientView must be flush with the top edge of the widget so that the
  // web contents can take up the entire screen in immersive fullscreen (with
  // or without the top-of-window views revealed). When in immersive fullscreen
  // and the top-of-window views are revealed, the TopContainerView paints the
  // window header by redirecting paints from its background to
  // BrowserNonClientFrameViewAsh.
  return bounds();
}

gfx::Rect BrowserNonClientFrameViewAsh::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  return client_bounds;
}

int BrowserNonClientFrameViewAsh::NonClientHitTest(const gfx::Point& point) {
  if (hosted_app_button_container_) {
    gfx::Point client_point(point);
    View::ConvertPointToTarget(this, hosted_app_button_container_,
                               &client_point);
    if (hosted_app_button_container_->HitTestPoint(client_point))
      return HTCLIENT;
  }

  const int hit_test = ash::FrameBorderNonClientHitTest(
      this, back_button_, caption_button_container_, point);

  // When the window is restored we want a large click target above the tabs
  // to drag the window, so redirect clicks in the tab's shadow to caption.
  if (hit_test == HTCLIENT && !frame()->IsMaximized() &&
      !frame()->IsFullscreen()) {
    gfx::Point client_point(point);
    View::ConvertPointToTarget(this, frame()->client_view(), &client_point);
    gfx::Rect tabstrip_bounds(browser_view()->tabstrip()->bounds());
    constexpr int kTabShadowHeight = 4;
    if (client_point.y() < tabstrip_bounds.y() + kTabShadowHeight)
      return HTCAPTION;
  }

  return hit_test;
}

void BrowserNonClientFrameViewAsh::GetWindowMask(const gfx::Size& size,
                                                 gfx::Path* window_mask) {
  // Aura does not use window masks.
}

void BrowserNonClientFrameViewAsh::ResetWindowControls() {
  caption_button_container_->SetVisible(true);
  caption_button_container_->ResetWindowControls();
  if (hosted_app_button_container_)
    hosted_app_button_container_->RefreshContentSettingViews();
}

void BrowserNonClientFrameViewAsh::UpdateWindowIcon() {
  if (window_icon_)
    window_icon_->SchedulePaint();
}

void BrowserNonClientFrameViewAsh::UpdateWindowTitle() {
  if (!frame()->IsFullscreen())
    frame_header_->SchedulePaintForTitle();
}

void BrowserNonClientFrameViewAsh::SizeConstraintsChanged() {}

void BrowserNonClientFrameViewAsh::ActivationChanged(bool active) {
  BrowserNonClientFrameView::ActivationChanged(active);

  const bool should_paint_as_active = ShouldPaintAsActive();
  frame_header_->SetPaintAsActive(should_paint_as_active);

  if (hosted_app_button_container_)
    hosted_app_button_container_->SetPaintAsActive(should_paint_as_active);

  if (frame_header_origin_text_)
    frame_header_origin_text_->SetPaintAsActive(should_paint_as_active);
}

///////////////////////////////////////////////////////////////////////////////
// views::View:

void BrowserNonClientFrameViewAsh::OnPaint(gfx::Canvas* canvas) {
  if (!ShouldPaint())
    return;

  const ash::FrameHeader::Mode header_mode =
      ShouldPaintAsActive() ? ash::FrameHeader::MODE_ACTIVE
                            : ash::FrameHeader::MODE_INACTIVE;
  frame_header_->PaintHeader(canvas, header_mode);

  if (browser_view()->IsToolbarVisible() &&
      !browser_view()->toolbar()->GetPreferredSize().IsEmpty() &&
      browser_view()->IsTabStripVisible()) {
    PaintToolbarBackground(canvas);
  }
}

void BrowserNonClientFrameViewAsh::Layout() {
  // The header must be laid out before computing |painted_height| because the
  // computation of |painted_height| for app and popup windows depends on the
  // position of the window controls.
  frame_header_->LayoutHeader();

  int painted_height = GetTopInset(false);
  if (browser_view()->IsTabStripVisible())
    painted_height += browser_view()->tabstrip()->GetPreferredSize().height();

  frame_header_->SetHeaderHeightForPainting(painted_height);

  if (profile_indicator_icon())
    LayoutIncognitoButton();
  BrowserNonClientFrameView::Layout();
  const bool immersive =
      browser_view()->immersive_mode_controller()->IsEnabled();
  const bool tab_strip_visible = browser_view()->IsTabStripVisible();
  // In immersive fullscreen mode, the top view inset property should be 0.
  const int inset =
      (tab_strip_visible || immersive) ? 0 : GetTopInset(/*restored=*/false);
  frame()->GetNativeWindow()->SetProperty(aura::client::kTopViewInset, inset);

  if (frame_header_origin_text_) {
    // Align the right side of the text with the left side of the caption
    // buttons.
    gfx::Size origin_text_preferred_size =
        frame_header_origin_text_->GetPreferredSize();
    int origin_text_width =
        std::min(width() - caption_button_container_->width(),
                 origin_text_preferred_size.width());
    gfx::Rect text_bounds(origin_text_width,
                          origin_text_preferred_size.height());
    SetRightSide(&text_bounds, caption_button_container_->x());
    AlignVerticalCenterWith(&text_bounds, caption_button_container_->bounds());
    frame_header_origin_text_->SetBoundsRect(text_bounds);
  }

  // The top right corner must be occupied by a caption button for easy mouse
  // access. This check is agnostic to RTL layout.
  DCHECK_EQ(caption_button_container_->y(), 0);
  DCHECK_EQ(caption_button_container_->bounds().right(), width());
}

const char* BrowserNonClientFrameViewAsh::GetClassName() const {
  return "BrowserNonClientFrameViewAsh";
}

void BrowserNonClientFrameViewAsh::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kTitleBar;
}

gfx::Size BrowserNonClientFrameViewAsh::GetMinimumSize() const {
  gfx::Size min_client_view_size(frame()->client_view()->GetMinimumSize());
  int min_width = std::max(frame_header_->GetMinimumHeaderWidth(),
                           min_client_view_size.width());
  if (browser_view()->IsTabStripVisible()) {
    // Ensure that the minimum width is enough to hold a minimum width tab strip
    // at its usual insets.
    const int min_tabstrip_width =
        browser_view()->tabstrip()->GetMinimumSize().width();
    min_width = std::max(
        min_width,
        min_tabstrip_width + GetTabStripLeftInset() + GetTabStripRightInset());
  }
  return gfx::Size(min_width, min_client_view_size.height());
}

void BrowserNonClientFrameViewAsh::ChildPreferredSizeChanged(
    views::View* child) {
  if (browser_view()->initialized()) {
    InvalidateLayout();
    frame()->GetRootView()->Layout();
  }
}

///////////////////////////////////////////////////////////////////////////////
// ash::CustomFrameHeader::AppearanceProvider:

SkColor BrowserNonClientFrameViewAsh::GetFrameHeaderColor(bool active) {
  return GetFrameColor(active);
}

gfx::ImageSkia BrowserNonClientFrameViewAsh::GetFrameHeaderImage(bool active) {
  return GetFrameImage(active);
}

gfx::ImageSkia BrowserNonClientFrameViewAsh::GetFrameHeaderOverlayImage(
    bool active) {
  return GetFrameOverlayImage(active);
}

bool BrowserNonClientFrameViewAsh::IsTabletMode() {
  return TabletModeClient::Get() &&
         TabletModeClient::Get()->tablet_mode_enabled();
}

///////////////////////////////////////////////////////////////////////////////
// ash::ShellObserver:

void BrowserNonClientFrameViewAsh::OnOverviewModeStarting() {
  in_overview_mode_ = true;

  // Update the window icon if needed so that overview mode can grab the icon
  // from kAppIconKey or kWindowIconKey to display.
  if (!frame()->GetNativeWindow()->GetProperty(
          aura::client::kHasOverviewIcon)) {
    frame()->UpdateWindowIcon();
  }

  frame()->GetNativeWindow()->SetProperty(aura::client::kTopViewColor,
                                          GetFrameColor());
  OnOverviewOrSplitviewModeChanged();
}

void BrowserNonClientFrameViewAsh::OnOverviewModeEnded() {
  in_overview_mode_ = false;
  OnOverviewOrSplitviewModeChanged();
}

///////////////////////////////////////////////////////////////////////////////
// ash::mojom::TabletModeClient:

void BrowserNonClientFrameViewAsh::OnTabletModeToggled(bool enabled) {
  if (!enabled && browser_view()->immersive_mode_controller()->IsRevealed()) {
    // Before updating the caption buttons state below (which triggers a
    // relayout), we want to move the caption buttons from the TopContainerView
    // back to this view.
    OnImmersiveRevealEnded();
  }

  caption_button_container_->UpdateCaptionButtonState(true /*=animate*/);

  if (enabled) {
    // Enter immersive mode if the feature is enabled and the widget is not
    // already in fullscreen mode. Popups that are not activated but not
    // minimized are still put in immersive mode, since they may still be
    // visible but not activated due to something transparent and/or not
    // fullscreen (ie. fullscreen launcher).
    if (!frame()->IsFullscreen() && !browser_view()->IsBrowserTypeNormal() &&
        !frame()->IsMinimized()) {
      browser_view()->immersive_mode_controller()->SetEnabled(true);
      return;
    }
  } else {
    // Exit immersive mode if the feature is enabled and the widget is not in
    // fullscreen mode.
    if (!frame()->IsFullscreen() && !browser_view()->IsBrowserTypeNormal()) {
      browser_view()->immersive_mode_controller()->SetEnabled(false);
      return;
    }
  }

  InvalidateLayout();
  // Can be null in tests.
  if (frame()->client_view())
    frame()->client_view()->InvalidateLayout();
  if (frame()->GetRootView())
    frame()->GetRootView()->Layout();
}

///////////////////////////////////////////////////////////////////////////////
// TabIconViewModel:

bool BrowserNonClientFrameViewAsh::ShouldTabIconViewAnimate() const {
  // Hosted apps use their app icon and shouldn't show a throbber.
  if (extensions::HostedAppBrowserController::IsForExperimentalHostedAppBrowser(
          browser_view()->browser())) {
    return false;
  }

  // This function is queried during the creation of the window as the
  // TabIconView we host is initialized, so we need to null check the selected
  // WebContents because in this condition there is not yet a selected tab.
  content::WebContents* current_tab = browser_view()->GetActiveWebContents();
  return current_tab && current_tab->IsLoading();
}

gfx::ImageSkia BrowserNonClientFrameViewAsh::GetFaviconForTabIconView() {
  views::WidgetDelegate* delegate = frame()->widget_delegate();
  return delegate ? delegate->GetWindowIcon() : gfx::ImageSkia();
}

void BrowserNonClientFrameViewAsh::EnabledStateChangedForCommand(int id,
                                                                 bool enabled) {
  if (id == IDC_BACK && back_button_)
    back_button_->SetEnabled(enabled);
}

void BrowserNonClientFrameViewAsh::OnSplitViewStateChanged(
    ash::mojom::SplitViewState current_state) {
  split_view_state_ = current_state;
  OnOverviewOrSplitviewModeChanged();
}

///////////////////////////////////////////////////////////////////////////////
// aura::WindowObserver:

void BrowserNonClientFrameViewAsh::OnWindowDestroying(aura::Window* window) {
  DCHECK_EQ(frame()->GetNativeWindow(), window);
  window->RemoveObserver(this);
}

void BrowserNonClientFrameViewAsh::OnWindowPropertyChanged(aura::Window* window,
                                                           const void* key,
                                                           intptr_t old) {
  DCHECK_EQ(frame()->GetNativeWindow(), window);
  if (key != aura::client::kShowStateKey)
    return;
  frame_header_->OnShowStateChanged(
      window->GetProperty(aura::client::kShowStateKey));
}

///////////////////////////////////////////////////////////////////////////////
// ImmersiveModeController::Observer:

void BrowserNonClientFrameViewAsh::OnImmersiveRevealStarted() {
  // The frame caption buttons use ink drop highlights and flood fill effects.
  // They make those buttons paint_to_layer. On immersive mode, the browser's
  // TopContainerView is also converted to paint_to_layer (see
  // ImmersiveModeControllerAsh::OnImmersiveRevealStarted()). In this mode, the
  // TopContainerView is responsible for painting this
  // BrowserNonClientFrameViewAsh (see TopContainerView::PaintChildren()).
  // However, BrowserNonClientFrameViewAsh is a sibling of TopContainerView not
  // a child. As a result, when the frame caption buttons are set to
  // paint_to_layer as a result of an ink drop effect, they will disappear.
  // https://crbug.com/840242. To fix this, we'll make the caption buttons
  // temporarily children of the TopContainerView while they're all painting to
  // their layers.
  browser_view()->top_container()->AddChildViewAt(caption_button_container_, 0);
  if (back_button_)
    browser_view()->top_container()->AddChildViewAt(back_button_, 0);

  browser_view()->top_container()->Layout();
}

void BrowserNonClientFrameViewAsh::OnImmersiveRevealEnded() {
  AddChildViewAt(caption_button_container_, 0);
  if (back_button_)
    AddChildView(back_button_);
  Layout();
}

void BrowserNonClientFrameViewAsh::OnImmersiveFullscreenExited() {
  OnImmersiveRevealEnded();
}

HostedAppButtonContainer*
BrowserNonClientFrameViewAsh::GetHostedAppButtonContainerForTesting() const {
  return hosted_app_button_container_;
}

///////////////////////////////////////////////////////////////////////////////
// BrowserNonClientFrameViewAsh, protected:

// BrowserNonClientFrameView:
AvatarButtonStyle BrowserNonClientFrameViewAsh::GetAvatarButtonStyle() const {
  // Ash doesn't support a profile switcher button.
  return AvatarButtonStyle::NONE;
}

///////////////////////////////////////////////////////////////////////////////
// BrowserNonClientFrameViewAsh, private:

int BrowserNonClientFrameViewAsh::GetTabStripRightInset() const {
  int inset = caption_button_container_->GetPreferredSize().width();

  // For Material Refresh, the end of the tabstrip contains empty space to
  // ensure the window remains draggable, which is sufficient padding to the
  // other tabstrip contents.
  using MD = ui::MaterialDesignController;
  constexpr int kTabstripRightSpacing = 10;
  if (!MD::IsRefreshUi())
    inset += kTabstripRightSpacing;

  return inset;
}

bool BrowserNonClientFrameViewAsh::UsePackagedAppHeaderStyle() const {
  // Use for non tabbed trusted source windows, e.g. Settings, as well as apps.
  const Browser* const browser = browser_view()->browser();
  return (!browser->is_type_tabbed() && browser->is_trusted_source()) ||
         browser->is_app();
}

bool BrowserNonClientFrameViewAsh::ShouldPaint() const {
  // We need to paint when the top-of-window views are revealed in immersive
  // fullscreen.
  ImmersiveModeController* immersive_mode_controller =
      browser_view()->immersive_mode_controller();
  if (immersive_mode_controller->IsEnabled())
    return immersive_mode_controller->IsRevealed();

  if (frame()->IsFullscreen())
    return false;

  // Do not paint for V1 apps in overview mode.
  return browser_view()->IsBrowserTypeNormal() || !in_overview_mode_;
}

void BrowserNonClientFrameViewAsh::OnOverviewOrSplitviewModeChanged() {
  if (in_overview_mode_ &&
      IsSnappedInSplitView(frame()->GetNativeWindow(), split_view_state_)) {
    caption_button_container_->SetVisible(true);
    if (window_icon_)
      window_icon_->SetVisible(true);
    if (back_button_)
      back_button_->SetVisible(true);
  } else {
    caption_button_container_->SetVisible(!in_overview_mode_);
    if (window_icon_)
      window_icon_->SetVisible(!in_overview_mode_);
    if (back_button_)
      back_button_->SetVisible(!in_overview_mode_);
  }
  // Schedule a paint to show or hide the header.
  SchedulePaint();
}

std::unique_ptr<ash::FrameHeader>
BrowserNonClientFrameViewAsh::CreateFrameHeader() {
  std::unique_ptr<ash::FrameHeader> header;

  Browser* browser = browser_view()->browser();
  if (!UsePackagedAppHeaderStyle()) {
    auto browser_frame_header = std::make_unique<ash::CustomFrameHeader>(
        frame(), this, this, !browser_view()->IsRegularOrGuestSession(),
        caption_button_container_);
    header = std::move(browser_frame_header);
  } else {
    std::unique_ptr<ash::DefaultFrameHeader> default_frame_header =
        std::make_unique<ash::DefaultFrameHeader>(frame(), this,
                                                  caption_button_container_);
    // TODO(alancutter): Move this branch into a new HostedAppFrameHeader class.
    if (extensions::HostedAppBrowserController::
            IsForExperimentalHostedAppBrowser(browser)) {
      SkColor active_color = ash::FrameCaptionButton::GetButtonColor(
          ash::FrameCaptionButton::ColorMode::kDefault,
          ash::kDefaultFrameColor);

      // Hosted apps apply a theme color if specified by the extension.
      base::Optional<SkColor> theme_color =
          browser->hosted_app_controller()->GetThemeColor();
      if (theme_color) {
        theme_color = SkColorSetA(theme_color.value(), SK_AlphaOPAQUE);
        default_frame_header->SetThemeColor(*theme_color);
        active_color = ash::FrameCaptionButton::GetButtonColor(
            ash::FrameCaptionButton::ColorMode::kThemed, *theme_color);
      }

      // Add the container for extra hosted app buttons (e.g app menu button).
      const float inactive_alpha_ratio =
          ash::FrameCaptionButton::GetInactiveButtonColorAlphaRatio();
      SkColor inactive_color =
          SkColorSetA(active_color, 255 * inactive_alpha_ratio);
      hosted_app_button_container_ = new HostedAppButtonContainer(
          browser_view(), active_color, inactive_color);
      caption_button_container_->AddChildViewAt(hosted_app_button_container_,
                                                0);

      // Add the origin text.
      frame_header_origin_text_ =
          std::make_unique<ash::FrameHeaderOriginText>(
              browser->hosted_app_controller()->GetFormattedUrlOrigin(),
              active_color, inactive_color)
              .release();
      AddChildView(frame_header_origin_text_);

      // Schedule the title bar animation.
      constexpr base::TimeDelta kTitlebarAnimationDelay =
          base::TimeDelta::FromMilliseconds(750);
      base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&BrowserNonClientFrameViewAsh::StartHostedAppAnimation,
                         weak_factory_.GetWeakPtr()),
          kTitlebarAnimationDelay);
    } else if (!browser->is_app()) {
      default_frame_header->SetFrameColors(BrowserFrameAsh::kMdWebUiFrameColor,
                                           BrowserFrameAsh::kMdWebUiFrameColor);
    }
    header = std::move(default_frame_header);
  }

  header->SetBackButton(back_button_);
  header->SetLeftHeaderView(window_icon_);
  return header;
}

void BrowserNonClientFrameViewAsh::StartHostedAppAnimation() {
  frame_header_origin_text_->StartSlideAnimation();
  hosted_app_button_container_->StartTitlebarAnimation(
      frame_header_origin_text_->AnimationDuration());
}
