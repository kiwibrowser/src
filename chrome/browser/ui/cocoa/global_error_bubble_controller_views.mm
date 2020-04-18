// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/global_error_bubble_controller.h"
#include "chrome/browser/ui/views/global_error_bubble_view.h"
#import "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/gfx/mac/coordinate_conversion.h"

GlobalErrorBubbleViewBase* ShowViewsGlobalErrorBubbleOnCocoaBrowser(
    NSPoint anchor,
    Browser* browser,
    const base::WeakPtr<GlobalErrorWithStandardBubble>& error) {
  gfx::Point anchor_point =
      gfx::ScreenPointFromNSPoint(ui::ConvertPointFromWindowToScreen(
          browser->window()->GetNativeWindow(), anchor));
  gfx::NativeView parent =
      platform_util::GetViewForWindow(browser->window()->GetNativeWindow());
  DCHECK(parent);

  GlobalErrorBubbleView* bubble_view =
      new GlobalErrorBubbleView(nullptr, gfx::Rect(anchor_point, gfx::Size()),
                                views::BubbleBorder::TOP_RIGHT, browser, error);
  bubble_view->set_parent_window(parent);
  views::BubbleDialogDelegateView::CreateBubble(bubble_view);
  bubble_view->GetWidget()->Show();
  KeepBubbleAnchored(bubble_view);
  return bubble_view;
}
