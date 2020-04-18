// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/internal_app/internal_app_item.h"

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "chrome/browser/ui/app_list/app_context_menu.h"
#include "chrome/browser/ui/app_list/internal_app/internal_app_metadata.h"
#include "ui/base/l10n/l10n_util.h"

// static
const char InternalAppItem::kItemType[] = "InternalAppItem";

InternalAppItem::InternalAppItem(
    Profile* profile,
    const app_list::AppListSyncableService::SyncItem* sync_item,
    const app_list::InternalApp& internal_app)
    : ChromeAppListItem(profile, internal_app.app_id) {
  SetIcon(app_list::GetIconForResourceId(internal_app.icon_resource_id,
                                         app_list::kTileIconSize));
  SetName(l10n_util::GetStringUTF8(internal_app.name_string_resource_id));
  if (sync_item && sync_item->item_ordinal.IsValid())
    UpdateFromSync(sync_item);
  else
    SetDefaultPositionIfApplicable();
}

InternalAppItem::~InternalAppItem() = default;

const char* InternalAppItem::GetItemType() const {
  return InternalAppItem::kItemType;
}

void InternalAppItem::Activate(int event_flags) {
  app_list::OpenInternalApp(id(), profile());
}

void InternalAppItem::GetContextMenuModel(GetMenuModelCallback callback) {
  if (!context_menu_) {
    context_menu_ = std::make_unique<app_list::AppContextMenu>(
        nullptr, profile(), id(), GetController());
  }
  context_menu_->GetMenuModel(std::move(callback));
}

app_list::AppContextMenu* InternalAppItem::GetAppContextMenu() {
  return context_menu_.get();
}
