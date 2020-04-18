// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/model/app_list_item.h"

#include "ash/app_list/model/app_list_item_observer.h"
#include "base/logging.h"

namespace app_list {

AppListItem::AppListItem(const std::string& id)
    : metadata_(ash::mojom::AppListItemMetadata::New(id,
                                                     std::string(),
                                                     std::string(),
                                                     std::string(),
                                                     syncer::StringOrdinal(),
                                                     false,
                                                     gfx::ImageSkia())),
      highlighted_(false),
      is_installing_(false),
      percent_downloaded_(-1) {}

AppListItem::~AppListItem() {
  for (auto& observer : observers_)
    observer.ItemBeingDestroyed();
}

void AppListItem::SetIcon(const gfx::ImageSkia& icon) {
  metadata_->icon = icon;
  metadata_->icon.EnsureRepsForSupportedScales();
  for (auto& observer : observers_)
    observer.ItemIconChanged();
}

void AppListItem::SetIsInstalling(bool is_installing) {
  if (is_installing_ == is_installing)
    return;

  is_installing_ = is_installing;
  for (auto& observer : observers_)
    observer.ItemIsInstallingChanged();
}

void AppListItem::SetPercentDownloaded(int percent_downloaded) {
  if (percent_downloaded_ == percent_downloaded)
    return;

  percent_downloaded_ = percent_downloaded;
  for (auto& observer : observers_)
    observer.ItemPercentDownloadedChanged();
}

void AppListItem::AddObserver(AppListItemObserver* observer) {
  observers_.AddObserver(observer);
}

void AppListItem::RemoveObserver(AppListItemObserver* observer) {
  observers_.RemoveObserver(observer);
}

const char* AppListItem::GetItemType() const {
  static const char* app_type = "";
  return app_type;
}

AppListItem* AppListItem::FindChildItem(const std::string& id) {
  return NULL;
}

size_t AppListItem::ChildItemCount() const {
  return 0;
}

bool AppListItem::CompareForTest(const AppListItem* other) const {
  return id() == other->id() && folder_id() == other->folder_id() &&
         name() == other->name() && short_name_ == other->short_name_ &&
         GetItemType() == other->GetItemType() &&
         position().Equals(other->position());
}

std::string AppListItem::ToDebugString() const {
  return id().substr(0, 8) + " '" + name() + "'" + " [" +
         position().ToDebugString() + "]";
}

// Protected methods

void AppListItem::SetName(const std::string& name) {
  if (metadata_->name == name && (short_name_.empty() || short_name_ == name))
    return;
  metadata_->name = name;
  short_name_.clear();
  for (auto& observer : observers_)
    observer.ItemNameChanged();
}

void AppListItem::SetNameAndShortName(const std::string& name,
                                      const std::string& short_name) {
  if (metadata_->name == name && short_name_ == short_name)
    return;
  metadata_->name = name;
  short_name_ = short_name;
  for (auto& observer : observers_)
    observer.ItemNameChanged();
}

}  // namespace app_list
