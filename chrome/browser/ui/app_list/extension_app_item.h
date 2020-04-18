// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_EXTENSION_APP_ITEM_H_
#define CHROME_BROWSER_UI_APP_LIST_EXTENSION_APP_ITEM_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/extensions/chrome_app_icon_delegate.h"
#include "chrome/browser/ui/app_list/app_context_menu_delegate.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service.h"
#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "chrome/browser/ui/extensions/extension_enable_flow_delegate.h"
#include "extensions/browser/extension_icon_image.h"
#include "ui/gfx/image/image_skia.h"

class AppListControllerDelegate;
class ExtensionEnableFlow;
class Profile;

namespace app_list {
class ExtensionAppContextMenu;
}

namespace extensions {
class Extension;
}

// ExtensionAppItem represents an extension app in app list.
class ExtensionAppItem : public ChromeAppListItem,
                         public extensions::ChromeAppIconDelegate,
                         public ExtensionEnableFlowDelegate,
                         public app_list::AppContextMenuDelegate {
 public:
  static const char kItemType[];

  ExtensionAppItem(Profile* profile,
                   AppListModelUpdater* model_updater,
                   const app_list::AppListSyncableService::SyncItem* sync_item,
                   const std::string& extension_id,
                   const std::string& extension_name,
                   const gfx::ImageSkia& installing_icon,
                   bool is_platform_app);
  ~ExtensionAppItem() override;

  // Reload the title and icon from the underlying extension.
  void Reload();

  // Update page and app launcher ordinals to put the app in between |prev| and
  // |next|. Note that |prev| and |next| could be NULL when the app is put at
  // the beginning or at the end.
  void Move(const ExtensionAppItem* prev, const ExtensionAppItem* next);

  const std::string& extension_id() const { return id(); }
  const std::string& extension_name() const { return extension_name_; }

 private:
  // Gets extension associated with this model. Returns NULL if extension
  // no longer exists.
  const extensions::Extension* GetExtension() const;

  // Checks if extension is disabled and if enable flow should be started.
  // Returns true if extension enable flow is started or there is already one
  // running.
  bool RunExtensionEnableFlow();

  // Private equivalent to Activate(), without refocus for already-running apps.
  void Launch(int event_flags);

  // Overridden from ExtensionEnableFlowDelegate:
  void ExtensionEnableFlowFinished() override;
  void ExtensionEnableFlowAborted(bool user_initiated) override;

  // Overridden from ChromeAppListItem:
  void Activate(int event_flags) override;
  void GetContextMenuModel(GetMenuModelCallback callback) override;
  const char* GetItemType() const override;
  bool IsBadged() const override;
  app_list::AppContextMenu* GetAppContextMenu() override;

  // Overridden from app_list::AppContextMenuDelegate:
  void ExecuteLaunchCommand(int event_flags) override;

  // extensions::ChromeAppIconDelegate:
  void OnIconUpdated(extensions::ChromeAppIcon* icon) override;

  std::unique_ptr<extensions::ChromeAppIcon> icon_;
  std::unique_ptr<app_list::ExtensionAppContextMenu> context_menu_;
  std::unique_ptr<ExtensionEnableFlow> extension_enable_flow_;
  AppListControllerDelegate* extension_enable_flow_controller_;

  // Name to use for the extension if we can't access it.
  std::string extension_name_;

  // Icon for the extension if we can't access the installed extension.
  const gfx::ImageSkia installing_icon_;

  // Whether or not this app is a platform app.
  bool is_platform_app_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionAppItem);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_EXTENSION_APP_ITEM_H_
