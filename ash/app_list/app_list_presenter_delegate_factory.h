// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_APP_LIST_PRESENTER_DELEGATE_FACTORY_H_
#define ASH_APP_LIST_APP_LIST_PRESENTER_DELEGATE_FACTORY_H_

#include <memory>

#include "ash/app_list/presenter/app_list_presenter_delegate_factory.h"
#include "ash/ash_export.h"
#include "base/macros.h"

namespace app_list {
class AppListViewDelegateFactory;
}

namespace ash {

class ASH_EXPORT AppListPresenterDelegateFactory
    : public app_list::AppListPresenterDelegateFactory {
 public:
  explicit AppListPresenterDelegateFactory(
      std::unique_ptr<app_list::AppListViewDelegateFactory>);
  ~AppListPresenterDelegateFactory() override;

  std::unique_ptr<app_list::AppListPresenterDelegate> GetDelegate(
      app_list::AppListPresenterImpl* presenter) override;

 private:
  std::unique_ptr<app_list::AppListViewDelegateFactory> view_delegate_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppListPresenterDelegateFactory);
};

}  // namespace ash

#endif  // ASH_APP_LIST_APP_LIST_PRESENTER_DELEGATE_FACTORY_H_
