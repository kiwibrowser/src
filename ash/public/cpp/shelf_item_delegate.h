// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_SHELF_ITEM_DELEGATE_H_
#define ASH_PUBLIC_CPP_SHELF_ITEM_DELEGATE_H_

#include <memory>
#include <string>

#include "ash/public/cpp/ash_public_export.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/events/event.h"

class AppWindowLauncherItemController;

namespace ui {
class MenuModel;
}

namespace ash {

using MenuItemList = std::vector<mojom::MenuItemPtr>;

// ShelfItemDelegate tracks some item state and serves as a base class for
// various subclasses that implement the mojo interface.
class ASH_PUBLIC_EXPORT ShelfItemDelegate : public mojom::ShelfItemDelegate {
 public:
  explicit ShelfItemDelegate(const ShelfID& shelf_id);
  ~ShelfItemDelegate() override;

  const ShelfID& shelf_id() const { return shelf_id_; }
  void set_shelf_id(const ShelfID& shelf_id) { shelf_id_ = shelf_id; }
  const std::string& app_id() const { return shelf_id_.app_id; }
  const std::string& launch_id() const { return shelf_id_.launch_id; }

  bool image_set_by_controller() const { return image_set_by_controller_; }
  void set_image_set_by_controller(bool image_set_by_controller) {
    image_set_by_controller_ = image_set_by_controller;
  }

  // Returns a pointer to this instance, to be used by remote shelf models, etc.
  mojom::ShelfItemDelegatePtr CreateInterfacePtrAndBind();

  // Returns items for the application menu; used for convenience and testing.
  virtual MenuItemList GetAppMenuItems(int event_flags);

  // Returns the context menu model; used to show ShelfItem context menus.
  using GetMenuModelCallback =
      base::OnceCallback<void(std::unique_ptr<ui::MenuModel>)>;
  virtual void GetContextMenu(int64_t display_id,
                              GetMenuModelCallback callback);

  // Returns nullptr if class is not AppWindowLauncherItemController.
  virtual AppWindowLauncherItemController* AsAppWindowLauncherItemController();

  // Attempts to execute a context menu command; returns true if it was run.
  bool ExecuteContextMenuCommand(int64_t command_id, int32_t event_flags);

  // mojom::ShelfItemDelegate:
  void GetContextMenuItems(int64_t display_id,
                           GetContextMenuItemsCallback callback) override;

 private:
  // Bound by GetContextMenu().
  void OnGetContextMenu(GetContextMenuItemsCallback callback,
                        std::unique_ptr<ui::MenuModel> menu_model);

  // The shelf id; empty if there is no app associated with the item.
  // Besides the application id, ShelfID also contains a launch id, which is an
  // id that can be passed to an app when launched in order to support multiple
  // shelf items per app. This id is used together with the app_id to uniquely
  // identify each shelf item that has the same app_id.
  ShelfID shelf_id_;

  // A binding used by remote shelf item delegate users.
  mojo::Binding<mojom::ShelfItemDelegate> binding_;

  // Set to true if the launcher item image has been set by the controller.
  bool image_set_by_controller_ = false;

  // The context menu model that was last shown for the associated shelf item.
  std::unique_ptr<ui::MenuModel> context_menu_;

  base::WeakPtrFactory<ShelfItemDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShelfItemDelegate);
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_SHELF_ITEM_DELEGATE_H_
