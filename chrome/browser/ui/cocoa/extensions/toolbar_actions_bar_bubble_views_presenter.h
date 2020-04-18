// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_TOOLBAR_ACTIONS_BAR_BUBBLE_VIEWS_PRESENTER_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_TOOLBAR_ACTIONS_BAR_BUBBLE_VIEWS_PRESENTER_H_

#import <Foundation/Foundation.h>

#include <memory>

#include "base/macros.h"
#include "ui/views/widget/widget_observer.h"

@class NSWindow;

namespace test {
class ToolbarActionsBarBubbleViewsPresenterTestApi;
}

@class BrowserActionsController;
class ToolbarActionsBarBubbleDelegate;
class ToolbarActionsBarBubbleViews;

// Helper class for showing a toolkit-views toolbar action bubble on a Cocoa
// browser.
class ToolbarActionsBarBubbleViewsPresenter : public views::WidgetObserver {
 public:
  explicit ToolbarActionsBarBubbleViewsPresenter(
      BrowserActionsController* owner);
  ~ToolbarActionsBarBubbleViewsPresenter() override;

  // Presents |bubble| attached to the provided browser |parent_window| at
  // |point_in_screen|. |anchored_to_action_view| indicates that the anchor is
  // a specific browser action view, rather than something more general.
  void PresentAt(std::unique_ptr<ToolbarActionsBarBubbleDelegate> bubble,
                 NSWindow* parent_window,
                 NSPoint point_in_screen,
                 bool anchored_to_action_view);

 private:
  friend class test::ToolbarActionsBarBubbleViewsPresenterTestApi;

  // WidgetObserver:
  void OnWidgetClosing(views::Widget* widget) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  BrowserActionsController* owner_;  // Weak. Owns |this|.

  // Weak. Owns by its Widget (observed by |this|).
  ToolbarActionsBarBubbleViews* active_bubble_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarActionsBarBubbleViewsPresenter);
};

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_TOOLBAR_ACTIONS_BAR_BUBBLE_VIEWS_PRESENTER_H_
