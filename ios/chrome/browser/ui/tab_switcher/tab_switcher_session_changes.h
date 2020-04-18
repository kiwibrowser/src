// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_SESSION_CHANGES_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_SESSION_CHANGES_H_

#include <vector>

// This structure represents the changes a session undergoes.
// It is used to update the UICollectionView showing a set of tabs.
class TabSwitcherSessionChanges {
 public:
  TabSwitcherSessionChanges(std::vector<size_t> const& tabHashesInInitialState,
                            std::vector<size_t> const& tabHashesInFinalState);
  ~TabSwitcherSessionChanges();
  TabSwitcherSessionChanges(const TabSwitcherSessionChanges& sessionChanges) =
      delete;
  TabSwitcherSessionChanges& operator=(
      const TabSwitcherSessionChanges& sessionChanges) = delete;

  std::vector<size_t> const& deletions() const { return deletions_; }

  std::vector<size_t> const& insertions() const { return insertions_; }

  std::vector<size_t> const& updates() const { return updates_; }

  bool HasChanges() const;

 private:
  // Those vectors contain indexes of tabs.
  // The indexes are relative to a tab model snapshot, or a distant session.
  // To be in accordance with the UICollectionView's |performBatchUpdates|
  // method:
  // -the indexes in |updates| are relative to the previous state of the
  // session.
  // -the indexes in |deletions| are relative to the previous state of the
  // session.
  // -the indexes in |insertions| are relative to the final state of the
  // session.
  std::vector<size_t> deletions_;
  std::vector<size_t> insertions_;
  std::vector<size_t> updates_;
};

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_SESSION_CHANGES_H_
