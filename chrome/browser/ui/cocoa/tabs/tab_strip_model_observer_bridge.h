// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_MODEL_OBSERVER_BRIDGE_H_
#define CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_MODEL_OBSERVER_BRIDGE_H_

#import <Foundation/Foundation.h>

#include "base/compiler_specific.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class TabStripModel;

namespace content {
class WebContents;
}

// A C++ bridge class to handle receiving notifications from the C++ tab strip
// model. When the caller allocates a bridge, it automatically registers for
// notifications from |model| and passes messages to |controller| via the
// informal protocol below. The owner of this object is responsible for deleting
// it (and thus unhooking notifications) before |controller| is destroyed.
class TabStripModelObserverBridge : public TabStripModelObserver {
 public:
  TabStripModelObserverBridge(TabStripModel* model, id controller);
  ~TabStripModelObserverBridge() override;

  // Overridden from TabStripModelObserver
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabClosingAt(TabStripModel* tab_strip_model,
                    content::WebContents* contents,
                    int index) override;
  void TabDetachedAt(content::WebContents* contents,
                     int index,
                     bool was_active) override;
  void TabDeactivated(content::WebContents* contents) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabSelectionChanged(TabStripModel* tab_strip_model,
                           const ui::ListSelectionModel& old_model) override;
  void TabMoved(content::WebContents* contents,
                int from_index,
                int to_index) override;
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  void TabBlockedStateChanged(content::WebContents* contents,
                              int index) override;
  void TabStripEmpty() override;
  void SetTabNeedsAttentionAt(int index, bool attention) override;

 private:
  id controller_;  // weak, owns me
  TabStripModel* model_;  // weak, owned by Browser
};

// A protocol which can be selectively implemented by any Cocoa object to
// receive updates about changes to a tab strip model. All methods are optional.
@protocol TabStripModelBridge
@optional
- (void)insertTabWithContents:(content::WebContents*)contents
                      atIndex:(NSInteger)index
                 inForeground:(bool)inForeground;
- (void)tabClosingWithContents:(content::WebContents*)contents
                       atIndex:(NSInteger)index;
- (void)tabDetachedWithContents:(content::WebContents*)contents
                        atIndex:(NSInteger)index;
- (void)tabDeactivatedWithContents:(content::WebContents*)contents;
- (void)activateTabWithContents:(content::WebContents*)newContents
               previousContents:(content::WebContents*)oldContents
                        atIndex:(NSInteger)index
                         reason:(int)reason;
- (void)tabMovedWithContents:(content::WebContents*)contents
                   fromIndex:(NSInteger)from
                     toIndex:(NSInteger)to;
- (void)tabChangedWithContents:(content::WebContents*)contents
                       atIndex:(NSInteger)index
                    changeType:(TabChangeType)change;
- (void)tabReplacedWithContents:(content::WebContents*)newContents
               previousContents:(content::WebContents*)oldContents
                        atIndex:(NSInteger)index;
- (void)tabPinnedStateChangedWithContents:(content::WebContents*)contents
                                  atIndex:(NSInteger)index;
- (void)tabBlockedStateChangedWithContents:(content::WebContents*)contents
                                   atIndex:(NSInteger)index;
- (void)tabStripEmpty;
- (void)tabSelectionChanged;
- (void)tabAtIndex:(NSInteger)index needsAttention:(bool)attention;
@end

#endif  // CHROME_BROWSER_UI_COCOA_TABS_TAB_STRIP_MODEL_OBSERVER_BRIDGE_H_
