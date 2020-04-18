// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_PRESENTER_TEST_APP_LIST_PRESENTER_IMPL_TEST_API_H_
#define ASH_APP_LIST_PRESENTER_TEST_APP_LIST_PRESENTER_IMPL_TEST_API_H_

#include <stdint.h>

#include "base/macros.h"

namespace app_list {
class AppListPresenterDelegate;
class AppListPresenterImpl;
class AppListView;

namespace test {

// Accesses private data from an AppListController for testing.
class AppListPresenterImplTestApi {
 public:
  explicit AppListPresenterImplTestApi(AppListPresenterImpl* presenter);

  AppListView* view();
  AppListPresenterDelegate* presenter_delegate();

  void NotifyVisibilityChanged(bool visible, int64_t display_id);
  void NotifyTargetVisibilityChanged(bool visible);

 private:
  AppListPresenterImpl* const presenter_;

  DISALLOW_COPY_AND_ASSIGN(AppListPresenterImplTestApi);
};

}  // namespace test
}  // namespace app_list

#endif  // ASH_APP_LIST_PRESENTER_TEST_APP_LIST_PRESENTER_IMPL_TEST_API_H_
