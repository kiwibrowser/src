// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_SEARCH_RESULT_ACTIONS_VIEW_DELEGATE_H_
#define UI_APP_LIST_VIEWS_SEARCH_RESULT_ACTIONS_VIEW_DELEGATE_H_

#include <stddef.h>

namespace app_list {

class SearchResultActionsViewDelegate {
 public:
  // Invoked when the action button represent the action at |index| is pressed
  // in SearchResultActionsView.
  virtual void OnSearchResultActionActivated(size_t index, int event_flags) = 0;

 protected:
  virtual ~SearchResultActionsViewDelegate() {}
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_SEARCH_RESULT_ACTIONS_VIEW_DELEGATE_H_
