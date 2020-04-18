// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LIFECYCLE_UNIT_SOURCE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LIFECYCLE_UNIT_SOURCE_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_source_base.h"
#include "chrome/browser/resource_coordinator/page_signal_receiver.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class TabStripModel;

namespace content {
class WebContents;
}

namespace resource_coordinator {

class TabLifecycleObserver;
class TabLifecycleUnitExternal;

// Creates and destroys LifecycleUnits as tabs are created and destroyed.
class TabLifecycleUnitSource : public BrowserListObserver,
                               public LifecycleUnitSourceBase,
                               public PageSignalObserver,
                               public TabStripModelObserver {
 public:
  TabLifecycleUnitSource();
  ~TabLifecycleUnitSource() override;

  static TabLifecycleUnitSource* GetInstance();

  // Returns the TabLifecycleUnitExternal instance associated with
  // |web_contents|, or nullptr if |web_contents| isn't a tab.
  TabLifecycleUnitExternal* GetTabLifecycleUnitExternal(
      content::WebContents* web_contents) const;

  // Adds / removes an observer that is notified when the discarded or auto-
  // discardable state of a tab changes.
  void AddTabLifecycleObserver(TabLifecycleObserver* observer);
  void RemoveTabLifecycleObserver(TabLifecycleObserver* observer);

  // Pretend that |tab_strip| is the TabStripModel of the focused window.
  void SetFocusedTabStripModelForTesting(TabStripModel* tab_strip);

  class TabLifecycleUnitHolder;

 private:
  friend class TabLifecycleUnitTest;
  friend class TabManagerTest;
  FRIEND_TEST_ALL_PREFIXES(TabLifecycleUnitSourceTest,
                           TabProactiveDiscardedByFrozenCallback);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest, TabManagerWasDiscarded);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           TabManagerWasDiscardedCrossSiteSubFrame);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           ProactiveFastShutdownSingleTabProcess);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           ProactiveFastShutdownSharedTabProcess);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           ProactiveFastShutdownWithUnloadHandler);
  FRIEND_TEST_ALL_PREFIXES(TabManagerTest,
                           ProactiveFastShutdownWithBeforeunloadHandler);
  class TabLifecycleUnit;

  // Returns the TabLifecycleUnit instance associated with |web_contents|, or
  // nullptr if |web_contents| isn't a tab.
  TabLifecycleUnit* GetTabLifecycleUnit(
      content::WebContents* web_contents) const;

  // Returns the TabStripModel of the focused browser window, if any.
  TabStripModel* GetFocusedTabStripModel() const;

  // Updates the focused TabLifecycleUnit.
  void UpdateFocusedTab();

  // Updates the focused TabLifecycleUnit to |new_focused_lifecycle_unit|.
  // TabInsertedAt() calls this directly instead of UpdateFocusedTab() because
  // the active WebContents of a TabStripModel isn't updated when
  // TabInsertedAt() is called.
  void UpdateFocusedTabTo(TabLifecycleUnit* new_focused_lifecycle_unit);

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabDetachedAt(content::WebContents* contents,
                     int index,
                     bool was_active) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;

  // BrowserListObserver:
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;

  // PageSignalObserver:
  void OnLifecycleStateChanged(content::WebContents* web_contents,
                               mojom::LifecycleState state) override;

  // Tracks the BrowserList and all TabStripModels.
  BrowserTabStripTracker browser_tab_strip_tracker_;

  // Pretend that this is the TabStripModel of the focused window, for testing.
  TabStripModel* focused_tab_strip_model_for_testing_ = nullptr;

  // The currently focused TabLifecycleUnit. Updated by UpdateFocusedTab().
  TabLifecycleUnit* focused_lifecycle_unit_ = nullptr;

  // Observers notified when the discarded or auto-discardable state of a tab
  // changes.
  base::ObserverList<TabLifecycleObserver> tab_lifecycle_observers_;

  DISALLOW_COPY_AND_ASSIGN(TabLifecycleUnitSource);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LIFECYCLE_UNIT_SOURCE_H_
