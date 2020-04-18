// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_PAGINATION_MODEL_OBSERVER_H_
#define UI_APP_LIST_PAGINATION_MODEL_OBSERVER_H_

#include "ui/app_list/app_list_export.h"

namespace app_list {

class APP_LIST_EXPORT PaginationModelObserver {
 public:
  // Invoked when the total number of page is changed.
  virtual void TotalPagesChanged() = 0;

  // Invoked when the selected page index is changed.
  virtual void SelectedPageChanged(int old_selected, int new_selected) = 0;

  // Invoked when a transition starts.
  virtual void TransitionStarted() = 0;

  // Invoked when the transition data is changed.
  virtual void TransitionChanged() = 0;

  // Invoked when a transition ends.
  virtual void TransitionEnded() = 0;

 protected:
  virtual ~PaginationModelObserver() {}
};

}  // namespace app_list

#endif  // UI_APP_LIST_PAGINATION_MODEL_OBSERVER_H_
