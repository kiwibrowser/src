// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_SEARCH_RESOURCE_MANAGER_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_SEARCH_RESOURCE_MANAGER_H_

#include "base/macros.h"

class AppListModelUpdater;
class Profile;

namespace app_list {

// Manages the strings and assets of the app-list search box.
class SearchResourceManager {
 public:
  SearchResourceManager(Profile* profile, AppListModelUpdater* model_updater);
  ~SearchResourceManager();

 private:
  AppListModelUpdater* model_updater_;

  DISALLOW_COPY_AND_ASSIGN(SearchResourceManager);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_SEARCH_RESOURCE_MANAGER_H_
