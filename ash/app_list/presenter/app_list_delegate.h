// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_PRESENTER_APP_LIST_DELEGATE_H_
#define ASH_APP_LIST_PRESENTER_APP_LIST_DELEGATE_H_

#include <stdint.h>

#include "ash/app_list/presenter/app_list_presenter_export.h"

namespace app_list {

class APP_LIST_PRESENTER_EXPORT AppListDelegate {
 public:
  // Called when the visility of the app list changes. |display_id| is the
  // id of the display containing the app list.
  virtual void OnAppListVisibilityChanged(bool visible, int64_t display_id) = 0;

 protected:
  virtual ~AppListDelegate() {}
};

}  // namespace app_list

#endif  // ASH_APP_LIST_PRESENTER_APP_LIST_DELEGATE_H_
