// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

using content::WebContents;

TabStripModelObserver::TabStripModelObserver() {
}

void TabStripModelObserver::TabInsertedAt(TabStripModel* tab_strip_model,
                                          WebContents* contents,
                                          int index,
                                          bool foreground) {
}

void TabStripModelObserver::TabClosingAt(TabStripModel* tab_strip_model,
                                         WebContents* contents,
                                         int index) {
}

void TabStripModelObserver::TabDetachedAt(WebContents* contents,
                                          int index,
                                          bool was_active) {}

void TabStripModelObserver::TabDeactivated(WebContents* contents) {
}

void TabStripModelObserver::ActiveTabChanged(WebContents* old_contents,
                                             WebContents* new_contents,
                                             int index,
                                             int reason) {
}

void TabStripModelObserver::TabSelectionChanged(
    TabStripModel* tab_strip_model,
    const ui::ListSelectionModel& model) {
}

void TabStripModelObserver::TabMoved(WebContents* contents,
                                     int from_index,
                                     int to_index) {
}

void TabStripModelObserver::TabChangedAt(WebContents* contents,
                                         int index,
                                         TabChangeType change_type) {
}

void TabStripModelObserver::TabReplacedAt(TabStripModel* tab_strip_model,
                                          WebContents* old_contents,
                                          WebContents* new_contents,
                                          int index) {
}

void TabStripModelObserver::TabPinnedStateChanged(
    TabStripModel* tab_strip_model,
    WebContents* contents,
    int index) {
}

void TabStripModelObserver::TabBlockedStateChanged(WebContents* contents,
                                                   int index) {
}

void TabStripModelObserver::TabStripEmpty() {
}

void TabStripModelObserver::WillCloseAllTabs() {
}

void TabStripModelObserver::CloseAllTabsCanceled() {
}

void TabStripModelObserver::SetTabNeedsAttentionAt(int index, bool attention) {}
