// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/presenter/test/app_list_presenter_impl_test_api.h"

#include "ash/app_list/app_list_presenter_impl.h"
#include "ui/app_list/views/app_list_view.h"

namespace app_list {
namespace test {

AppListPresenterImplTestApi::AppListPresenterImplTestApi(
    AppListPresenterImpl* presenter)
    : presenter_(presenter) {}

AppListView* AppListPresenterImplTestApi::view() {
  return presenter_->view_;
}

AppListPresenterDelegate* AppListPresenterImplTestApi::presenter_delegate() {
  return presenter_->presenter_delegate_.get();
}

void AppListPresenterImplTestApi::NotifyVisibilityChanged(bool visible,
                                                          int64_t display_id) {
  presenter_->NotifyVisibilityChanged(visible, display_id);
}

void AppListPresenterImplTestApi::NotifyTargetVisibilityChanged(bool visible) {
  presenter_->NotifyTargetVisibilityChanged(visible);
}

}  // namespace test
}  // namespace app_list
