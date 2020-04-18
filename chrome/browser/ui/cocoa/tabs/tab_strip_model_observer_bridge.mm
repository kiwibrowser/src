// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/tabs/tab_strip_model_observer_bridge.h"

#include "chrome/browser/ui/tabs/tab_strip_model.h"

using content::WebContents;

TabStripModelObserverBridge::TabStripModelObserverBridge(TabStripModel* model,
                                                         id controller)
    : controller_(controller), model_(model) {
  DCHECK(model && controller);
  // Register to be a listener on the model so we can get updates and tell
  // |controller_| about them in the future.
  model_->AddObserver(this);
}

TabStripModelObserverBridge::~TabStripModelObserverBridge() {
  // Remove ourselves from receiving notifications.
  model_->RemoveObserver(this);
}

void TabStripModelObserverBridge::TabInsertedAt(TabStripModel* tab_strip_model,
                                                WebContents* contents,
                                                int index,
                                                bool foreground) {
  if ([controller_ respondsToSelector:
          @selector(insertTabWithContents:atIndex:inForeground:)]) {
    [controller_ insertTabWithContents:contents
                               atIndex:index
                          inForeground:foreground];
  }
}

void TabStripModelObserverBridge::TabClosingAt(TabStripModel* tab_strip_model,
                                               WebContents* contents,
                                               int index) {
  if ([controller_ respondsToSelector:
          @selector(tabClosingWithContents:atIndex:)]) {
    [controller_ tabClosingWithContents:contents atIndex:index];
  }
}

void TabStripModelObserverBridge::TabDetachedAt(WebContents* contents,
                                                int index,
                                                bool was_active) {
  if ([controller_ respondsToSelector:
          @selector(tabDetachedWithContents:atIndex:)]) {
    [controller_ tabDetachedWithContents:contents atIndex:index];
  }
}

void TabStripModelObserverBridge::TabDeactivated(WebContents* contents) {
  if ([controller_ respondsToSelector:@selector(tabDeactivatedWithContents:)])
    [controller_ tabDeactivatedWithContents:contents];
}

void TabStripModelObserverBridge::ActiveTabChanged(WebContents* old_contents,
                                                   WebContents* new_contents,
                                                   int index,
                                                   int reason) {
  if ([controller_ respondsToSelector:@selector(
          activateTabWithContents:previousContents:atIndex:reason:)]) {
    [controller_ activateTabWithContents:new_contents
                        previousContents:old_contents
                                 atIndex:index
                                  reason:reason];
  }
}

void TabStripModelObserverBridge::TabSelectionChanged(
    TabStripModel* tab_strip_model,
    const ui::ListSelectionModel& old_model) {
  if ([controller_ respondsToSelector:@selector(tabSelectionChanged)]) {
    [controller_ tabSelectionChanged];
  }
}

void TabStripModelObserverBridge::TabMoved(WebContents* contents,
                                           int from_index,
                                           int to_index) {
  if ([controller_ respondsToSelector:
       @selector(tabMovedWithContents:fromIndex:toIndex:)]) {
    [controller_ tabMovedWithContents:contents
                            fromIndex:from_index
                              toIndex:to_index];
  }
}

void TabStripModelObserverBridge::TabChangedAt(WebContents* contents,
                                               int index,
                                               TabChangeType change_type) {
  if ([controller_ respondsToSelector:
          @selector(tabChangedWithContents:atIndex:changeType:)]) {
    [controller_ tabChangedWithContents:contents
                                atIndex:index
                             changeType:change_type];
  }
}

void TabStripModelObserverBridge::TabReplacedAt(TabStripModel* tab_strip_model,
                                                WebContents* old_contents,
                                                WebContents* new_contents,
                                                int index) {
  if ([controller_ respondsToSelector:
          @selector(tabReplacedWithContents:previousContents:atIndex:)]) {
    [controller_ tabReplacedWithContents:new_contents
                        previousContents:old_contents
                                 atIndex:index];
  } else {
    TabChangedAt(new_contents, index, TabChangeType::kAll);
  }
}

void TabStripModelObserverBridge::TabPinnedStateChanged(
    TabStripModel* tab_strip_model,
    WebContents* contents,
    int index) {
  if ([controller_ respondsToSelector:
          @selector(tabPinnedStateChangedWithContents:atIndex:)]) {
    [controller_ tabPinnedStateChangedWithContents:contents
                                           atIndex:index];
  }
}

void TabStripModelObserverBridge::TabBlockedStateChanged(WebContents* contents,
                                                         int index) {
  if ([controller_ respondsToSelector:@selector
                   (tabBlockedStateChangedWithContents:atIndex:)]) {
    [controller_ tabBlockedStateChangedWithContents:contents atIndex:index];
  }
}

void TabStripModelObserverBridge::TabStripEmpty() {
  if ([controller_ respondsToSelector:@selector(tabStripEmpty)])
    [controller_ tabStripEmpty];
}

void TabStripModelObserverBridge::SetTabNeedsAttentionAt(int index,
                                                         bool attention) {
  if ([controller_ respondsToSelector:@selector(tabAtIndex:needsAttention:)])
    [controller_ tabAtIndex:index needsAttention:attention];
}
