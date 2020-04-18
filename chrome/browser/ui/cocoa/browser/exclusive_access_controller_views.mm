// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser/exclusive_access_controller_views.h"

#include "chrome/browser/download/download_shelf.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/accelerators_cocoa.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/status_bubble.h"
#include "chrome/browser/ui/views/exclusive_access_bubble_views.h"
#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_host.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/gfx/mac/coordinate_conversion.h"
#include "ui/views/event_monitor.h"

namespace {

// If |callback| was never passed to |ExclusiveAccessBubbleViews|, calls it
// with |kNotShown|, otherwise does nothing.
void CallHideCallbackAsNotShownIfNecessary(
    ExclusiveAccessBubbleHideCallback callback) {
  if (callback)
    std::move(callback).Run(ExclusiveAccessBubbleHideReason::kNotShown);
}

}  // anonymous namespace

ExclusiveAccessController::ExclusiveAccessController(
    BrowserWindowController* controller,
    Browser* browser)
    : controller_(controller),
      browser_(browser),
      bubble_type_(EXCLUSIVE_ACCESS_BUBBLE_TYPE_NONE) {
  pref_registrar_.Init(GetProfile()->GetPrefs());
  pref_registrar_.Add(
      prefs::kShowFullscreenToolbar,
      base::Bind(&ExclusiveAccessController::UpdateFullscreenToolbar,
                 base::Unretained(this)));
}

ExclusiveAccessController::~ExclusiveAccessController() {
  CallHideCallbackAsNotShownIfNecessary(std::move(bubble_first_hide_callback_));
}

void ExclusiveAccessController::Show() {
  views_bubble_.reset(new ExclusiveAccessBubbleViews(
      this, url_, bubble_type_, std::move(bubble_first_hide_callback_)));
}

void ExclusiveAccessController::Destroy() {
  views_bubble_.reset();
  url_ = GURL();
  bubble_type_ = EXCLUSIVE_ACCESS_BUBBLE_TYPE_NONE;
  CallHideCallbackAsNotShownIfNecessary(std::move(bubble_first_hide_callback_));
}

Profile* ExclusiveAccessController::GetProfile() {
  return browser_->profile();
}

bool ExclusiveAccessController::IsFullscreen() const {
  return [controller_ isInAnyFullscreenMode];
}

void ExclusiveAccessController::UpdateUIForTabFullscreen(
    TabFullscreenState state) {
  [controller_ updateUIForTabFullscreen:state];
}

void ExclusiveAccessController::UpdateFullscreenToolbar() {
  [[controller_ fullscreenToolbarController]
      layoutToolbarStyleIsExitingTabFullscreen:NO];
}

// See the Fullscreen terminology section and the (Fullscreen) interface
// category in browser_window_controller.h for a detailed explanation of the
// logic in this method.
void ExclusiveAccessController::EnterFullscreen(
    const GURL& url,
    ExclusiveAccessBubbleType bubble_type) {
  url_ = url;
  bubble_type_ = bubble_type;
  CallHideCallbackAsNotShownIfNecessary(std::move(bubble_first_hide_callback_));
  if (browser_->exclusive_access_manager()
          ->fullscreen_controller()
          ->IsWindowFullscreenForTabOrPending())
    [controller_ enterWebContentFullscreen];
  else
    [controller_ enterBrowserFullscreen];

  // This is not guarded by FullscreenControlHost::IsFullscreenExitUIEnabled()
  // because ShouldHideUIForFullscreen() already guards that mouse and touch
  // inputs will not trigger the exit UI, and we always need
  // FullscreenControlHost whenever keyboard lock requires press-and-hold ESC.
  fullscreen_control_host_event_monitor_ =
      views::EventMonitor::CreateWindowMonitor(
          GetFullscreenControlHost(),
          GetActiveWebContents()->GetTopLevelNativeWindow());
}

void ExclusiveAccessController::ExitFullscreen() {
  [controller_ exitAnyFullscreen];
  if (fullscreen_control_host_) {
    fullscreen_control_host_->Hide(false);
    fullscreen_control_host_event_monitor_.reset();
  }
}

void ExclusiveAccessController::UpdateExclusiveAccessExitBubbleContent(
    const GURL& url,
    ExclusiveAccessBubbleType bubble_type,
    ExclusiveAccessBubbleHideCallback bubble_first_hide_callback,
    bool force_update) {
  url_ = url;
  bubble_type_ = bubble_type;
  CallHideCallbackAsNotShownIfNecessary(std::move(bubble_first_hide_callback_));
  bubble_first_hide_callback_ = std::move(bubble_first_hide_callback);
  [controller_ updateFullscreenExitBubble];
}

void ExclusiveAccessController::OnExclusiveAccessUserInput() {
  if (views_bubble_)
    views_bubble_->OnUserInput();
}

content::WebContents* ExclusiveAccessController::GetActiveWebContents() {
  return browser_->tab_strip_model()->GetActiveWebContents();
}

void ExclusiveAccessController::UnhideDownloadShelf() {
  GetBrowserWindow()->GetDownloadShelf()->Unhide();
}

void ExclusiveAccessController::HideDownloadShelf() {
  GetBrowserWindow()->GetDownloadShelf()->Hide();
  StatusBubble* statusBubble = GetBrowserWindow()->GetStatusBubble();
  if (statusBubble)
    statusBubble->Hide();
}

bool ExclusiveAccessController::ShouldHideUIForFullscreen() const {
  return false;
}

ExclusiveAccessBubbleViews*
ExclusiveAccessController::GetExclusiveAccessBubble() {
  return views_bubble_.get();
}

bool ExclusiveAccessController::GetAcceleratorForCommandId(
    int cmd_id,
    ui::Accelerator* accelerator) const {
  *accelerator =
      *AcceleratorsCocoa::GetInstance()->GetAcceleratorForCommand(cmd_id);
  return true;
}

ExclusiveAccessManager* ExclusiveAccessController::GetExclusiveAccessManager() {
  return browser_->exclusive_access_manager();
}

views::Widget* ExclusiveAccessController::GetBubbleAssociatedWidget() {
  NOTREACHED();  // Only used for non-simplified UI.
  return nullptr;
}

ui::AcceleratorProvider* ExclusiveAccessController::GetAcceleratorProvider() {
  return this;
}

gfx::NativeView ExclusiveAccessController::GetBubbleParentView() const {
  return [[controller_ window] contentView];
}

gfx::Point ExclusiveAccessController::GetCursorPointInParent() const {
  NSWindow* window = [controller_ window];
  NSPoint location =
      ui::ConvertPointFromScreenToWindow(window, [NSEvent mouseLocation]);
  return gfx::Point(location.x,
                    NSHeight([[window contentView] frame]) - location.y);
}

gfx::Rect ExclusiveAccessController::GetClientAreaBoundsInScreen() const {
  return gfx::ScreenRectFromNSRect([[controller_ window] frame]);
}

bool ExclusiveAccessController::IsImmersiveModeEnabled() const {
  return false;
}

gfx::Rect ExclusiveAccessController::GetTopContainerBoundsInScreen() {
  NOTREACHED();  // Only used for ImmersiveMode.
  return gfx::Rect();
}

void ExclusiveAccessController::DestroyAnyExclusiveAccessBubble() {
  Destroy();
}

bool ExclusiveAccessController::CanTriggerOnMouse() const {
  return true;
}

BrowserWindow* ExclusiveAccessController::GetBrowserWindow() const {
  return [controller_ browserWindow];
}

FullscreenControlHost* ExclusiveAccessController::GetFullscreenControlHost() {
  if (!fullscreen_control_host_) {
    fullscreen_control_host_ =
        std::make_unique<FullscreenControlHost>(this, this);
  }

  return fullscreen_control_host_.get();
}
