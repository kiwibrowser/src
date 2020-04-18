// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/toolbar_actions_bar_bubble_views_presenter.h"

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_bubble_delegate.h"
#include "chrome/browser/ui/views/toolbar/toolbar_actions_bar_bubble_views.h"
#import "ui/gfx/mac/coordinate_conversion.h"
#include "ui/views/widget/widget.h"

ToolbarActionsBarBubbleViewsPresenter::ToolbarActionsBarBubbleViewsPresenter(
    BrowserActionsController* owner)
    : owner_(owner), active_bubble_(nullptr) {}

ToolbarActionsBarBubbleViewsPresenter::
    ~ToolbarActionsBarBubbleViewsPresenter() {
  // Child windows should never outlive the controller that owns |this|.
  DCHECK(!active_bubble_)
      << "|active_bubble_| should be cleared by OnWidgetDestroying().";
}

void ToolbarActionsBarBubbleViewsPresenter::PresentAt(
    std::unique_ptr<ToolbarActionsBarBubbleDelegate> delegate,
    NSWindow* parent_window,
    NSPoint point_in_screen,
    bool anchored_to_action_view) {
  gfx::Point views_point = gfx::ScreenPointFromNSPoint(point_in_screen);
  // For a Cocoa browser, the presenter must pass nullptr for |anchor_view|.
  ToolbarActionsBarBubbleViews* bubble = new ToolbarActionsBarBubbleViews(
      nullptr, views_point, anchored_to_action_view, std::move(delegate));
  bubble->set_parent_window([parent_window contentView]);
  active_bubble_ = bubble;
  views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->GetWidget()->AddObserver(this);
  bubble->Show();
  KeepBubbleAnchored(bubble);
}

void ToolbarActionsBarBubbleViewsPresenter::OnWidgetClosing(
    views::Widget* widget) {
  // OnWidgetClosing() gives an earlier signal than OnWidgetDestroying() but is
  // not called when the bubble is closed synchronously. Note the observer is
  // removed so it's impossible for both methods to be triggered.
  OnWidgetDestroying(widget);
}

void ToolbarActionsBarBubbleViewsPresenter::OnWidgetDestroying(
    views::Widget* widget) {
  active_bubble_ = nullptr;
  [owner_ bubbleWindowClosing:nil];
  widget->RemoveObserver(this);
}
