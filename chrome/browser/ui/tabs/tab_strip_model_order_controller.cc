// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_strip_model_order_controller.h"

#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"

///////////////////////////////////////////////////////////////////////////////
// TabStripModelOrderController, public:

TabStripModelOrderController::TabStripModelOrderController(
    TabStripModel* tabstrip)
    : tabstrip_(tabstrip) {
  tabstrip_->AddObserver(this);
}

TabStripModelOrderController::~TabStripModelOrderController() {
  tabstrip_->RemoveObserver(this);
}

int TabStripModelOrderController::DetermineInsertionIndex(
    ui::PageTransition transition,
    bool foreground) {
  int tab_count = tabstrip_->count();
  if (!tab_count)
    return 0;

  // NOTE: TabStripModel enforces that all non-mini-tabs occur after mini-tabs,
  // so we don't have to check here too.
  if (ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_LINK) &&
      tabstrip_->active_index() != -1) {
    if (foreground) {
      // If the page was opened in the foreground by a link click in another
      // tab, insert it adjacent to the tab that opened that link.
      return tabstrip_->active_index() + 1;
    }
    content::WebContents* opener = tabstrip_->GetActiveWebContents();
    // Get the index of the next item opened by this tab, and insert after
    // it...
    int index = tabstrip_->GetIndexOfLastWebContentsOpenedBy(
        opener, tabstrip_->active_index());
    if (index != TabStripModel::kNoTab)
      return index + 1;
    // Otherwise insert adjacent to opener...
    return tabstrip_->active_index() + 1;
  }
  // In other cases, such as Ctrl+T, open at the end of the strip.
  return tabstrip_->count();
}

int TabStripModelOrderController::DetermineNewSelectedIndex(
    int removing_index) const {
  int tab_count = tabstrip_->count();
  DCHECK(removing_index >= 0 && removing_index < tab_count);
  content::WebContents* parent_opener =
      tabstrip_->GetOpenerOfWebContentsAt(removing_index);
  // First see if the index being removed has any "child" tabs. If it does, we
  // want to select the first in that child group, not the next tab in the same
  // group of the removed tab.
  content::WebContents* removed_contents =
      tabstrip_->GetWebContentsAt(removing_index);
  // The parent opener should never be the same as the controller being removed.
  DCHECK(parent_opener != removed_contents);
  int index = tabstrip_->GetIndexOfNextWebContentsOpenedBy(removed_contents,
                                                           removing_index,
                                                           false);
  if (index != TabStripModel::kNoTab)
    return GetValidIndex(index, removing_index);

  if (parent_opener) {
    // If the tab was in a group, shift selection to the next tab in the group.
    int index = tabstrip_->GetIndexOfNextWebContentsOpenedBy(parent_opener,
                                                             removing_index,
                                                             false);
    if (index != TabStripModel::kNoTab)
      return GetValidIndex(index, removing_index);

    // If we can't find a subsequent group member, just fall back to the
    // parent_opener itself. Note that we use "group" here since opener is
    // reset by select operations..
    index = tabstrip_->GetIndexOfWebContents(parent_opener);
    if (index != TabStripModel::kNoTab)
      return GetValidIndex(index, removing_index);
  }

  // No opener set, fall through to the default handler...
  int selected_index = tabstrip_->active_index();
  if (selected_index >= (tab_count - 1))
    return selected_index - 1;

  return selected_index;
}

void TabStripModelOrderController::ActiveTabChanged(
    content::WebContents* old_contents,
    content::WebContents* new_contents,
    int index,
    int reason) {
  content::WebContents* old_opener = NULL;
  if (old_contents) {
    int index = tabstrip_->GetIndexOfWebContents(old_contents);
    if (index != TabStripModel::kNoTab) {
      old_opener = tabstrip_->GetOpenerOfWebContentsAt(index);

      // Forget any group/opener relationships that need to be reset whenever
      // selection changes (see comment in TabStripModel::AddWebContentsAt).
      if (tabstrip_->ShouldResetGroupOnSelect(old_contents))
        tabstrip_->ForgetGroup(old_contents);
    }
  }
  content::WebContents* new_opener = tabstrip_->GetOpenerOfWebContentsAt(index);

  if ((reason & CHANGE_REASON_USER_GESTURE) && new_opener != old_opener &&
      ((old_contents == NULL && new_opener == NULL) ||
          new_opener != old_contents) &&
      ((new_contents == NULL && old_opener == NULL) ||
          old_opener != new_contents)) {
    tabstrip_->ForgetAllOpeners();
  }
}

///////////////////////////////////////////////////////////////////////////////
// TabStripModelOrderController, private:

int TabStripModelOrderController::GetValidIndex(
    int index, int removing_index) const {
  if (removing_index < index)
    index = std::max(0, index - 1);
  return index;
}
