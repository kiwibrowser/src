// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_MODEL_APP_LIST_VIEW_STATE_H_
#define ASH_APP_LIST_MODEL_APP_LIST_VIEW_STATE_H_

namespace app_list {

enum class AppListViewState {
  // Closes |app_list_main_view_| and dismisses the delegate.
  CLOSED = 0,
  // The initial state for the app list when neither maximize or side shelf
  // modes are active. If set, the widget will peek over the shelf by
  // kPeekingAppListHeight DIPs.
  PEEKING = 1,
  // Entered when text is entered into the search box from peeking mode.
  HALF = 2,
  // Default app list state in maximize and side shelf modes. Entered from an
  // upward swipe from |PEEKING| or from clicking the chevron.
  FULLSCREEN_ALL_APPS = 3,
  // Entered from an upward swipe from |HALF| or by entering text in the
  // search box from |FULLSCREEN_ALL_APPS|.
  FULLSCREEN_SEARCH = 4,
};

}  // namespace app_list

#endif  // ASH_APP_LIST_MODEL_APP_LIST_VIEW_STATE_H_
