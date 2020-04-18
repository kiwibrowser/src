// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_ITEM_H_
#define CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_ITEM_H_

#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "chrome/browser/ui/app_list/crostini/crostini_app_icon.h"

class CrostiniAppContextMenu;

class CrostiniAppItem : public ChromeAppListItem,
                        public CrostiniAppIcon::Observer {
 public:
  static const char kItemType[];
  CrostiniAppItem(Profile* profile,
                  AppListModelUpdater* model_updater,
                  const app_list::AppListSyncableService::SyncItem* sync_item,
                  const std::string& id,
                  const std::string& name);
  ~CrostiniAppItem() override;

  CrostiniAppIcon* crostini_app_icon() { return crostini_app_icon_.get(); }

  // CrostiniAppIcon::Observer
  void OnIconUpdated(CrostiniAppIcon* icon) override;

 private:
  // ChromeAppListItem:
  void Activate(int event_flags) override;
  const char* GetItemType() const override;
  void GetContextMenuModel(GetMenuModelCallback callback) override;
  app_list::AppContextMenu* GetAppContextMenu() override;

  void UpdateIcon();

  std::unique_ptr<CrostiniAppIcon> crostini_app_icon_;
  std::unique_ptr<CrostiniAppContextMenu> context_menu_;

  DISALLOW_COPY_AND_ASSIGN(CrostiniAppItem);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_CROSTINI_CROSTINI_APP_ITEM_H_
