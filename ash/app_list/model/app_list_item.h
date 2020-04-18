// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_MODEL_APP_LIST_ITEM_H_
#define ASH_APP_LIST_MODEL_APP_LIST_ITEM_H_

#include <stddef.h>

#include <string>
#include <utility>

#include "ash/app_list/model/app_list_model_export.h"
#include "ash/public/interfaces/app_list.mojom.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/sync/model/string_ordinal.h"
#include "ui/gfx/image/image_skia.h"

class FastShowPickler;

namespace ash {
class AppListControllerImpl;
}  // namespace ash

namespace app_list {

class AppListItemList;
class AppListItemListTest;
class AppListItemObserver;
class AppListModel;

// AppListItem provides icon and title to be shown in a AppListItemView
// and action to be executed when the AppListItemView is activated.
class APP_LIST_MODEL_EXPORT AppListItem {
 public:
  explicit AppListItem(const std::string& id);
  virtual ~AppListItem();

  void SetIcon(const gfx::ImageSkia& icon);
  const gfx::ImageSkia& icon() const { return metadata_->icon; }

  const std::string& GetDisplayName() const {
    return short_name_.empty() ? name() : short_name_;
  }

  const std::string& name() const { return metadata_->name; }
  // Should only be used in tests; otherwise use GetDisplayName().
  const std::string& short_name() const { return short_name_; }

  void set_highlighted(bool highlighted) { highlighted_ = highlighted; }
  bool highlighted() const { return highlighted_; }

  void SetIsInstalling(bool is_installing);
  bool is_installing() const { return is_installing_; }

  void SetPercentDownloaded(int percent_downloaded);
  int percent_downloaded() const { return percent_downloaded_; }

  bool IsInFolder() const { return !folder_id().empty(); }

  const std::string& id() const { return metadata_->id; }
  const std::string& folder_id() const { return metadata_->folder_id; }
  const syncer::StringOrdinal& position() const { return metadata_->position; }

  void SetMetadata(ash::mojom::AppListItemMetadataPtr metadata) {
    metadata_ = std::move(metadata);
  }
  const ash::mojom::AppListItemMetadata* GetMetadata() const {
    return metadata_.get();
  }
  ash::mojom::AppListItemMetadataPtr CloneMetadata() const {
    return metadata_.Clone();
  }

  void AddObserver(AppListItemObserver* observer);
  void RemoveObserver(AppListItemObserver* observer);

  // Returns a static const char* identifier for the subclass (defaults to "").
  // Pointers can be compared for quick type checking.
  virtual const char* GetItemType() const;

  // Returns the item matching |id| contained in this item (e.g. if the item is
  // a folder), or NULL if the item was not found or this is not a container.
  virtual AppListItem* FindChildItem(const std::string& id);

  // Returns the number of child items if it has any (e.g. is a folder) or 0.
  virtual size_t ChildItemCount() const;

  // Utility functions for sync integration tests.
  virtual bool CompareForTest(const AppListItem* other) const;
  virtual std::string ToDebugString() const;

  bool is_folder() const { return metadata_->is_folder; }

 protected:
  friend class ::FastShowPickler;
  friend class ash::AppListControllerImpl;
  friend class AppListItemList;
  friend class AppListItemListTest;
  friend class AppListModel;

  // These should only be called by AppListModel or in tests so that name
  // changes trigger update notifications.

  // Sets the full name of the item. Clears any shortened name.
  void SetName(const std::string& name);

  // Sets the full name and an optional shortened name of the item (e.g. to use
  // if the full name is too long to fit in a view).
  void SetNameAndShortName(const std::string& name,
                           const std::string& short_name);

  void set_position(const syncer::StringOrdinal& new_position) {
    DCHECK(new_position.IsValid());
    metadata_->position = new_position;
  }

  void set_folder_id(const std::string& folder_id) {
    metadata_->folder_id = folder_id;
  }

  void set_is_folder(bool is_folder) { metadata_->is_folder = is_folder; }

 private:
  friend class AppListModelTest;

  ash::mojom::AppListItemMetadataPtr metadata_;

  // A shortened name for the item, used for display.
  std::string short_name_;

  bool highlighted_;
  bool is_installing_;
  int percent_downloaded_;

  base::ObserverList<AppListItemObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(AppListItem);
};

}  // namespace app_list

#endif  // ASH_APP_LIST_MODEL_APP_LIST_ITEM_H_
