// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_INTERNAL_APP_RESULT_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_INTERNAL_APP_RESULT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/app_list/search/app_result.h"

class AppListControllerDelegate;
class Profile;

namespace app_list {

class AppContextMenu;

class InternalAppResult : public AppResult {
 public:
  InternalAppResult(Profile* profile,
                    const std::string& app_id,
                    AppListControllerDelegate* controller,
                    bool is_recommendation);
  ~InternalAppResult() override;

  // ChromeSearchResult overrides:
  void Open(int event_flags) override;
  void GetContextMenuModel(GetMenuModelCallback callback) override;

  // AppContextMenuDelegate overrides:
  void ExecuteLaunchCommand(int event_flags) override;

 private:
  // ChromeSearchResult overrides:
  AppContextMenu* GetAppContextMenu() override;

  std::unique_ptr<AppContextMenu> context_menu_;

  DISALLOW_COPY_AND_ASSIGN(InternalAppResult);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_INTERNAL_APP_RESULT_H_
