// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_

#include "base/macros.h"
#include "chrome/browser/ui/tabs/tab_change_type.h"

class TabStripModel;

namespace content {
class WebContents;
}

namespace ui {
class ListSelectionModel;
}

////////////////////////////////////////////////////////////////////////////////
//
// TabStripModelObserver
//
//  Objects implement this interface when they wish to be notified of changes
//  to the TabStripModel.
//
//  Two major implementers are the TabStrip, which uses notifications sent
//  via this interface to update the presentation of the strip, and the Browser
//  object, which updates bookkeeping and shows/hides individual WebContentses.
//
//  Register your TabStripModelObserver with the TabStripModel using its
//  Add/RemoveObserver methods.
//
////////////////////////////////////////////////////////////////////////////////
class TabStripModelObserver {
 public:
  enum ChangeReason {
    // Used to indicate that none of the reasons below are responsible for the
    // active tab change.
    CHANGE_REASON_NONE = 0,
    // The active tab changed because the tab's web contents was replaced.
    CHANGE_REASON_REPLACED = 1 << 0,
    // The active tab changed due to a user input event.
    CHANGE_REASON_USER_GESTURE = 1 << 1,
  };

  // A new WebContents was inserted into the TabStripModel at the
  // specified index. |foreground| is whether or not it was opened in the
  // foreground (selected).
  virtual void TabInsertedAt(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index,
                             bool foreground);

  // The specified WebContents at |index| is being closed (and eventually
  // destroyed). |tab_strip_model| is the TabStripModel that contained the tab.
  // TODO(erikchen): |index| is not used outside of tests. Do not use it. It
  // will be removed soon. https://crbug.com/842194.
  virtual void TabClosingAt(TabStripModel* tab_strip_model,
                            content::WebContents* contents,
                            int index);

  // The specified WebContents at |previous_index| has been detached, perhaps to
  // be inserted in another TabStripModel. The implementer should take whatever
  // action is necessary to deal with the WebContents no longer being present.
  // |previous_index| cannot be used to index into the current state of the
  // TabStripModel.
  virtual void TabDetachedAt(content::WebContents* contents,
                             int previous_index,
                             bool was_active);

  // The active WebContents is about to change from |old_contents|.
  // This gives observers a chance to prepare for an impending switch before it
  // happens.
  virtual void TabDeactivated(content::WebContents* contents);

  // Sent when the active tab changes. The previously active tab is identified
  // by |old_contents| and the newly active tab by |new_contents|. |index| is
  // the index of |new_contents|. If |reason| has CHANGE_REASON_REPLACED set
  // then the web contents was replaced (see TabChangedAt). If |reason| has
  // CHANGE_REASON_USER_GESTURE set then the web contents was changed due to a
  // user input event (e.g. clicking on a tab, keystroke).
  // Note: It is possible for the selection to change while the active tab
  // remains unchanged. For example, control-click may not change the active tab
  // but does change the selection. In this case |ActiveTabChanged| is not sent.
  // If you care about any changes to the selection, override
  // TabSelectionChanged.
  // Note: |old_contents| will be NULL if there was no contents previously
  // active.
  virtual void ActiveTabChanged(content::WebContents* old_contents,
                                content::WebContents* new_contents,
                                int index,
                                int reason);

  // Sent when the selection changes in |tab_strip_model|. More precisely when
  // selected tabs, anchor tab or active tab change. |old_model| is a snapshot
  // of the selection model before the change. See also ActiveTabChanged for
  // details.
  // TODO(erikchen): |old_model| is not used outside of tests. Do not use it. It
  // will be removed soon. https://crbug.com/842194.
  virtual void TabSelectionChanged(TabStripModel* tab_strip_model,
                                   const ui::ListSelectionModel& old_model);

  // The specified WebContents at |from_index| was moved to |to_index|.
  virtual void TabMoved(content::WebContents* contents,
                        int from_index,
                        int to_index);

  // The specified WebContents at |index| changed in some way. |contents|
  // may be an entirely different object and the old value is no longer
  // available by the time this message is delivered.
  //
  // See tab_change_type.h for a description of |change_type|.
  virtual void TabChangedAt(content::WebContents* contents,
                            int index,
                            TabChangeType change_type);

  // The WebContents was replaced at the specified index. This is invoked when
  // prerendering swaps in a prerendered WebContents.
  virtual void TabReplacedAt(TabStripModel* tab_strip_model,
                             content::WebContents* old_contents,
                             content::WebContents* new_contents,
                             int index);

  // Invoked when the pinned state of a tab changes.
  virtual void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                     content::WebContents* contents,
                                     int index);

  // Invoked when the blocked state of a tab changes.
  // NOTE: This is invoked when a tab becomes blocked/unblocked by a tab modal
  // window.
  virtual void TabBlockedStateChanged(content::WebContents* contents,
                                      int index);

  // The TabStripModel now no longer has any tabs. The implementer may
  // use this as a trigger to try and close the window containing the
  // TabStripModel, for example...
  virtual void TabStripEmpty();

  // Sent any time an attempt is made to close all the tabs. This is not
  // necessarily the result of CloseAllTabs(). For example, if the user closes
  // the last tab WillCloseAllTabs() is sent. If the close does not succeed
  // during the current event (say unload handlers block it) then
  // CloseAllTabsCanceled() is sent. Also note that if the last tab is detached
  // (DetachWebContentsAt()) then this is not sent.
  virtual void WillCloseAllTabs();
  virtual void CloseAllTabsCanceled();

  // The specified tab at |index| requires the display of a UI indication to the
  // user that it needs their attention. The UI indication is set iff
  // |attention| is true.
  virtual void SetTabNeedsAttentionAt(int index, bool attention);

 protected:
  TabStripModelObserver();
  virtual ~TabStripModelObserver() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TabStripModelObserver);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_
