// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/dev_tools_controller.h"

#include <algorithm>
#include <cmath>

#include <Cocoa/Cocoa.h>

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/cocoa/base_view.h"
#include "ui/base/cocoa/focus_tracker.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/mac/scoped_cocoa_disable_screen_updates.h"

using content::WebContents;

@interface DevToolsContainerView : BaseView {
  DevToolsContentsResizingStrategy strategy_;

  // Weak references. Ownership via -subviews.
  NSView* devToolsView_;
  NSView* contentsView_;
}

// Returns true iff layout has changed.
- (BOOL)setDevToolsView:(NSView*)devToolsView
           withStrategy:(const DevToolsContentsResizingStrategy&)strategy;
- (void)adjustSubviews;
- (BOOL)hasDevToolsView;

@end


@implementation DevToolsContainerView

- (BOOL)setDevToolsView:(NSView*)devToolsView
           withStrategy:(const DevToolsContentsResizingStrategy&)strategy {
  BOOL strategy_changed = !strategy_.Equals(strategy);
  strategy_.CopyFrom(strategy);
  if (devToolsView == devToolsView_) {
    if (contentsView_)
      [contentsView_ setHidden:strategy.hide_inspected_contents()];
    return strategy_changed;
  }

  if (devToolsView_) {
    DCHECK_EQ(2u, [[self subviews] count]);
    [devToolsView_ removeFromSuperview];
    [contentsView_ setHidden:NO];
    contentsView_ = nil;
    devToolsView_ = nil;
  }

  if (devToolsView) {
    NSArray* subviews = [self subviews];
    DCHECK_EQ(1u, [subviews count]);
    contentsView_ = [subviews objectAtIndex:0];
    devToolsView_ = devToolsView;
    // Place DevTools under contents.
    [self addSubview:devToolsView positioned:NSWindowBelow relativeTo:nil];

    [contentsView_ setHidden:strategy.hide_inspected_contents()];
  }

  return YES;
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  [self adjustSubviews];
}

- (BOOL)hasDevToolsView {
  return devToolsView_ != nil;
}

- (void)adjustSubviews {
  if (![[self subviews] count])
    return;

  if (!devToolsView_) {
    DCHECK_EQ(1u, [[self subviews] count]);
    NSView* contents = [[self subviews] objectAtIndex:0];
    [contents setFrame:[self bounds]];
    return;
  }

  DCHECK_EQ(2u, [[self subviews] count]);

  gfx::Rect new_devtools_bounds;
  gfx::Rect new_contents_bounds;
  ApplyDevToolsContentsResizingStrategy(
      strategy_, gfx::Size(NSSizeToCGSize([self bounds].size)),
      &new_devtools_bounds, &new_contents_bounds);
  [devToolsView_ setFrame:[self flipRectToNSRect:new_devtools_bounds]];
  [contentsView_ setFrame:[self flipRectToNSRect:new_contents_bounds]];
}

@end


@implementation DevToolsController

- (id)init {
  if ((self = [super init])) {
    devToolsContainerView_.reset(
        [[DevToolsContainerView alloc] initWithFrame:NSZeroRect]);
    [devToolsContainerView_
        setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
  }
  return self;
}

- (NSView*)view {
  return devToolsContainerView_.get();
}

- (BOOL)updateDevToolsForWebContents:(WebContents*)contents
                         withProfile:(Profile*)profile {
  DevToolsContentsResizingStrategy strategy;
  WebContents* devTools = DevToolsWindow::GetInTabWebContents(
      contents, &strategy);

  // Make sure we do not draw any transient arrangements of views.
  gfx::ScopedCocoaDisableScreenUpdates disabler;

  if (devTools && ![devToolsContainerView_ hasDevToolsView]) {
    focusTracker_.reset(
        [[FocusTracker alloc] initWithWindow:[devToolsContainerView_ window]]);
  }

  if (!devTools && [devToolsContainerView_ hasDevToolsView]) {
    [focusTracker_ restoreFocusInWindow:[devToolsContainerView_ window]];
    focusTracker_.reset();
  }

  NSView* devToolsView = nil;
  if (devTools) {
    devToolsView = devTools->GetNativeView();
    // |devToolsView| is a WebContentsViewCocoa object, whose ViewID was
    // set to VIEW_ID_TAB_CONTAINER initially, so we need to change it to
    // VIEW_ID_DEV_TOOLS_DOCKED here.
    view_id_util::SetID(devToolsView, VIEW_ID_DEV_TOOLS_DOCKED);

    devTools->SetAllowOtherViews(true);
    contents->SetAllowOtherViews(true);
  } else {
    contents->SetAllowOtherViews(false);
  }

  BOOL result = [devToolsContainerView_ setDevToolsView:devToolsView
                                           withStrategy:strategy];
  [devToolsContainerView_ adjustSubviews];
  return result;
}

@end
