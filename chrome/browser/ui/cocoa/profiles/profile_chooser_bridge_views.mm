// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/profiles/profile_chooser_bridge_views.h"

#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile_metrics.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_base_controller.h"
#include "chrome/browser/ui/views/profiles/profile_chooser_view.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/gfx/mac/coordinate_conversion.h"
#include "ui/views/widget/widget_observer.h"

namespace {

// Offset needed to align the edge of the profile chooser bubble with the edge
// of the avatar button.
constexpr gfx::Insets kBubbleInsets(1, 1, 0, 1);

}  // namespace

ProfileChooserViewBridge::ProfileChooserViewBridge(
    AvatarBaseController* controller,
    views::Widget* bubble_widget)
    : scoped_observer_(this), controller_(controller) {
  scoped_observer_.Add(bubble_widget);
}

ProfileChooserViewBridge::~ProfileChooserViewBridge() = default;

void ProfileChooserViewBridge::OnWidgetDestroying(
    views::Widget* bubble_widget) {
  [controller_ bubbleWillClose];
  // Note: |this| is deleted here.
}

std::unique_ptr<ProfileChooserViewBridge> ShowProfileChooserViews(
    AvatarBaseController* avatar_base_controller,
    NSView* anchor,
    signin_metrics::AccessPoint access_point,
    Browser* browser,
    profiles::BubbleViewMode bubble_view_mode) {
  NSRect rect = [anchor convertRect:[anchor bounds] toView:nil];
  rect.origin =
      ui::ConvertPointFromWindowToScreen([anchor window], rect.origin);
  gfx::Rect anchor_rect = gfx::ScreenRectFromNSRect(rect);
  // Adjusting the offset by reducing x and width for RTL and LTR.
  anchor_rect.Inset(kBubbleInsets);
  gfx::NativeView anchor_window =
      platform_util::GetViewForWindow(browser->window()->GetNativeWindow());
  // Select the first other profile if the menu is opened by the keyboard
  // shortcut.
  bool is_source_keyboard = [[NSApp currentEvent] type] == NSKeyDown;
  ProfileChooserView::ShowBubble(
      bubble_view_mode, signin::ManageAccountsParams(), access_point, nullptr,
      anchor_window, anchor_rect, browser, is_source_keyboard);
  ProfileMetrics::LogProfileOpenMethod(ProfileMetrics::ICON_AVATAR_BUBBLE);
  std::unique_ptr<ProfileChooserViewBridge> bridge(new ProfileChooserViewBridge(
      avatar_base_controller, ProfileChooserView::GetCurrentBubbleWidget()));
  return bridge;
}
