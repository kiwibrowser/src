// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/app_list_client_impl.h"

#include <memory>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/app_list/app_list_model_updater.h"
#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "chrome/browser/ui/app_list/test/chrome_app_list_test_support.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/constants.h"
#include "ui/base/models/menu_model.h"

// Browser Test for AppListClientImpl.
using AppListClientImplBrowserTest = InProcessBrowserTest;

// Test that all the items in the context menu for a hosted app have valid
// labels.
IN_PROC_BROWSER_TEST_F(AppListClientImplBrowserTest, ShowContextMenu) {
  AppListClientImpl* client = AppListClientImpl::GetInstance();
  EXPECT_TRUE(client);

  // Show the app list to ensure it has loaded a profile.
  client->ShowAppList();
  AppListModelUpdater* model_updater = test::GetModelUpdater(client);
  EXPECT_TRUE(model_updater);

  // Get the webstore hosted app, which is always present.
  ChromeAppListItem* item = model_updater->FindItem(extensions::kWebStoreAppId);
  EXPECT_TRUE(item);

  base::RunLoop run_loop;
  std::unique_ptr<ui::MenuModel> menu_model;
  item->GetContextMenuModel(base::BindLambdaForTesting(
      [&](std::unique_ptr<ui::MenuModel> created_menu) {
        menu_model = std::move(created_menu);
        run_loop.Quit();
      }));
  run_loop.Run();
  EXPECT_TRUE(menu_model);

  int num_items = menu_model->GetItemCount();
  EXPECT_LT(0, num_items);

  for (int i = 0; i < num_items; i++) {
    if (menu_model->GetTypeAt(i) == ui::MenuModel::TYPE_SEPARATOR)
      continue;

    base::string16 label = menu_model->GetLabelAt(i);
    EXPECT_FALSE(label.empty());
  }
}
