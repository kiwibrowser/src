// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_session_changes.h"

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_utils.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TabSwitcherSessionChanges::TabSwitcherSessionChanges(
    std::vector<size_t> const& tabHashesInInitialState,
    std::vector<size_t> const& tabHashesInFinalState) {
  TabSwitcherMinimalReplacementOperations(tabHashesInInitialState,
                                          tabHashesInFinalState, &updates_,
                                          &deletions_, &insertions_);
}

TabSwitcherSessionChanges::~TabSwitcherSessionChanges() {}

bool TabSwitcherSessionChanges::HasChanges() const {
  return updates_.size() || deletions_.size() || insertions_.size();
}
