// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/model/app_list_folder_item.h"

#include "ash/app_list/model/app_list_item_list.h"
#include "base/guid.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"

namespace app_list {

AppListFolderItem::AppListFolderItem(const std::string& id)
    : AppListItem(id),
      folder_type_(id == ash::kOemFolderId ? FOLDER_TYPE_OEM
                                           : FOLDER_TYPE_NORMAL),
      item_list_(new AppListItemList),
      folder_image_(item_list_.get()) {
  folder_image_.AddObserver(this);
  set_is_folder(true);
}

AppListFolderItem::~AppListFolderItem() {
  folder_image_.RemoveObserver(this);
}

gfx::Rect AppListFolderItem::GetTargetIconRectInFolderForItem(
    AppListItem* item,
    const gfx::Rect& folder_icon_bounds) {
  return folder_image_.GetTargetIconRectInFolderForItem(item,
                                                        folder_icon_bounds);
}

// static
const char AppListFolderItem::kItemType[] = "FolderItem";

const char* AppListFolderItem::GetItemType() const {
  return AppListFolderItem::kItemType;
}

AppListItem* AppListFolderItem::FindChildItem(const std::string& id) {
  return item_list_->FindItem(id);
}

size_t AppListFolderItem::ChildItemCount() const {
  return item_list_->item_count();
}

bool AppListFolderItem::CompareForTest(const AppListItem* other) const {
  if (!AppListItem::CompareForTest(other))
    return false;
  const AppListFolderItem* other_folder =
      static_cast<const AppListFolderItem*>(other);
  if (other_folder->item_list()->item_count() != item_list_->item_count())
    return false;
  for (size_t i = 0; i < item_list_->item_count(); ++i) {
    if (!item_list()->item_at(i)->CompareForTest(
            other_folder->item_list()->item_at(i)))
      return false;
  }
  return true;
}

std::string AppListFolderItem::GenerateId() {
  return base::GenerateGUID();
}

void AppListFolderItem::OnFolderImageUpdated() {
  SetIcon(folder_image_.icon());
}

}  // namespace app_list
