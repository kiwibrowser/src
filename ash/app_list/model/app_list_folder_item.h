// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_MODEL_APP_LIST_FOLDER_ITEM_H_
#define ASH_APP_LIST_MODEL_APP_LIST_FOLDER_ITEM_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "ash/app_list/model/app_list_item.h"
#include "ash/app_list/model/app_list_item_list_observer.h"
#include "ash/app_list/model/app_list_item_observer.h"
#include "ash/app_list/model/app_list_model_export.h"
#include "ash/app_list/model/folder_image.h"
#include "base/macros.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace app_list {

class AppListItemList;

// AppListFolderItem implements the model/controller for folders.
class APP_LIST_MODEL_EXPORT AppListFolderItem : public AppListItem,
                                                public FolderImageObserver {
 public:
  // The folder type affects folder behavior.
  enum FolderType {
    // Default folder type.
    FOLDER_TYPE_NORMAL,
    // Items can not be moved to/from OEM folders in the UI.
    FOLDER_TYPE_OEM
  };

  static const char kItemType[];

  explicit AppListFolderItem(const std::string& id);
  ~AppListFolderItem() override;

  // Returns the target icon bounds for |item| to fly back to its parent folder
  // icon in animation UI. If |item| is one of the top item icon, this will
  // match its corresponding top item icon in the folder icon. Otherwise,
  // the target icon bounds is centered at the |folder_icon_bounds| with
  // the same size of the top item icon.
  // The Rect returned is in the same coordinates of |folder_icon_bounds|.
  gfx::Rect GetTargetIconRectInFolderForItem(
      AppListItem* item,
      const gfx::Rect& folder_icon_bounds);

  AppListItemList* item_list() { return item_list_.get(); }
  const AppListItemList* item_list() const { return item_list_.get(); }

  // For tests.
  const FolderImage& folder_image() const { return folder_image_; }

  FolderType folder_type() const { return folder_type_; }

  // AppListItem overrides:
  const char* GetItemType() const override;
  AppListItem* FindChildItem(const std::string& id) override;
  size_t ChildItemCount() const override;
  bool CompareForTest(const AppListItem* other) const override;

  // Returns an id for a new folder.
  static std::string GenerateId();

  // FolderImageObserver overrides:
  void OnFolderImageUpdated() override;

 private:
  // The type of folder; may affect behavior of folder views.
  const FolderType folder_type_;

  // List of items in the folder.
  std::unique_ptr<AppListItemList> item_list_;

  FolderImage folder_image_;

  DISALLOW_COPY_AND_ASSIGN(AppListFolderItem);
};

}  // namespace app_list

#endif  // ASH_APP_LIST_MODEL_APP_LIST_FOLDER_ITEM_H_
