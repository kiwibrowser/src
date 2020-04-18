// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/crostini/crostini_app_item.h"

#include <utility>

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "base/bind.h"
#include "chrome/browser/chromeos/crostini/crostini_util.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/crostini/crostini_app_context_menu.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "content/public/browser/browser_thread.h"

// static
const char CrostiniAppItem::kItemType[] = "CrostiniAppItem";

CrostiniAppItem::CrostiniAppItem(
    Profile* profile,
    AppListModelUpdater* model_updater,
    const app_list::AppListSyncableService::SyncItem* sync_item,
    const std::string& id,
    const std::string& name)
    : ChromeAppListItem(profile, id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  crostini_app_icon_.reset(
      new CrostiniAppIcon(profile, id, app_list::kTileIconSize, this));

  SetName(name);
  UpdateIcon();
  if (sync_item && sync_item->item_ordinal.IsValid()) {
    UpdateFromSync(sync_item);
  } else {
    SetDefaultPositionIfApplicable();
  }

  // Set model updater last to avoid being called during construction.
  set_model_updater(model_updater);
}

CrostiniAppItem::~CrostiniAppItem() {}

const char* CrostiniAppItem::GetItemType() const {
  return CrostiniAppItem::kItemType;
}

void CrostiniAppItem::Activate(int event_flags) {
  ChromeLauncherController::instance()->ActivateApp(
      id(), ash::LAUNCH_FROM_APP_LIST, event_flags);

  // TODO(timloh): Launching Crostini apps can take a few seconds if the
  // container is not currently running. Hiding the launcher at least provides
  // the user some feedback that they actually clicked an icon. We should make
  // this better, e.g. by showing some sort of spinner. We also need to handle
  // failures to start the container or app, as those are currently ignored.
  if (!GetController()->IsHomeLauncherEnabledInTabletMode())
    GetController()->DismissView();
}

void CrostiniAppItem::GetContextMenuModel(GetMenuModelCallback callback) {
  context_menu_ = std::make_unique<CrostiniAppContextMenu>(profile(), id(),
                                                           GetController());
  context_menu_->GetMenuModel(std::move(callback));
}

app_list::AppContextMenu* CrostiniAppItem::GetAppContextMenu() {
  return context_menu_.get();
}

void CrostiniAppItem::UpdateIcon() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  SetIcon(crostini_app_icon_->image_skia());
}

void CrostiniAppItem::OnIconUpdated(CrostiniAppIcon* icon) {
  UpdateIcon();
}
