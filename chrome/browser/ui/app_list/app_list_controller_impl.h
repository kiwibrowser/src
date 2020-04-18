// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_APP_LIST_CONTROLLER_IMPL_H_
#define CHROME_BROWSER_UI_APP_LIST_APP_LIST_CONTROLLER_IMPL_H_

#include <string>

#include "ash/public/cpp/shelf_types.h"
#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "ui/display/types/display_constants.h"

namespace ash {
namespace mojom {
class AppListController;
}  // namespace mojom
}  // namespace ash

class AppListControllerDelegateImpl : public AppListControllerDelegate {
 public:
  AppListControllerDelegateImpl();
  ~AppListControllerDelegateImpl() override;

  // AppListControllerDelegate overrides:
  void DismissView() override;
  int64_t GetAppListDisplayId() override;
  void SetAppListDisplayId(int64_t display_id) override;
  void GetAppInfoDialogBounds(GetAppInfoDialogBoundsCallback callback) override;
  bool IsAppPinned(const std::string& app_id) override;
  bool IsAppOpen(const std::string& app_id) const override;
  void PinApp(const std::string& app_id) override;
  void UnpinApp(const std::string& app_id) override;
  Pinnable GetPinnable(const std::string& app_id) override;
  void OnShowChildDialog() override;
  void OnCloseChildDialog() override;
  void CreateNewWindow(Profile* profile, bool incognito) override;
  void OpenURL(Profile* profile,
               const GURL& url,
               ui::PageTransition transition,
               WindowOpenDisposition disposition) override;
  void ActivateApp(Profile* profile,
                   const extensions::Extension* extension,
                   AppListSource source,
                   int event_flags) override;
  void LaunchApp(Profile* profile,
                 const extensions::Extension* extension,
                 AppListSource source,
                 int event_flags,
                 int64_t display_id) override;

  // Sets the pointer to the app list controller in Ash.
  void SetAppListController(ash::mojom::AppListController* app_list_controller);

 private:
  ash::ShelfLaunchSource AppListSourceToLaunchSource(AppListSource source);

  // The current display id showing the app list.
  int64_t display_id_ = display::kInvalidDisplayId;

  // Not owned.
  ash::mojom::AppListController* app_list_controller_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AppListControllerDelegateImpl);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_APP_LIST_CONTROLLER_IMPL_H_
