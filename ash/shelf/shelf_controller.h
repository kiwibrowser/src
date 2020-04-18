// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_CONTROLLER_H_
#define ASH_SHELF_SHELF_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/public/cpp/shelf_item.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/shelf_model_observer.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "ash/session/session_observer.h"
#include "ash/wm/tablet_mode/tablet_mode_observer.h"
#include "base/scoped_observer.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "ui/message_center/message_center_observer.h"

class PrefChangeRegistrar;
class PrefRegistrySimple;

namespace message_center {
class MessageCenter;
}

namespace ash {

// Ash's ShelfController owns the ShelfModel and implements interface functions
// that allow Chrome to modify and observe the Shelf and ShelfModel state.
// Chrome keeps its own ShelfModel copy in sync with Ash's ShelfModel.
class ASH_EXPORT ShelfController : public message_center::MessageCenterObserver,
                                   public mojom::ShelfController,
                                   public ShelfModelObserver,
                                   public SessionObserver,
                                   public TabletModeObserver,
                                   public WindowTreeHostManager::Observer {
 public:
  ShelfController();
  ~ShelfController() override;

  // Removes observers from this object's dependencies.
  void Shutdown();

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Binds the mojom::ShelfController interface request to this object.
  void BindRequest(mojom::ShelfControllerRequest request);

  ShelfModel* model() { return &model_; }

  // mojom::ShelfController:
  void AddObserver(mojom::ShelfObserverAssociatedPtrInfo observer) override;
  void AddShelfItem(int32_t index, const ShelfItem& item) override;
  void RemoveShelfItem(const ShelfID& id) override;
  void MoveShelfItem(const ShelfID& id, int32_t index) override;
  void UpdateShelfItem(const ShelfItem& item) override;
  void SetShelfItemDelegate(const ShelfID& id,
                            mojom::ShelfItemDelegatePtr delegate) override;

  // ShelfModelObserver:
  void ShelfItemAdded(int index) override;
  void ShelfItemRemoved(int index, const ShelfItem& old_item) override;
  void ShelfItemMoved(int start_index, int target_index) override;
  void ShelfItemChanged(int index, const ShelfItem& old_item) override;
  void ShelfItemDelegateChanged(const ShelfID& id,
                                ShelfItemDelegate* old_delegate,
                                ShelfItemDelegate* delegate) override;

  // MessageCenterObserver:
  void OnNotificationAdded(const std::string& notification_id) override;
  void OnNotificationRemoved(const std::string& notification_id,
                             bool by_user) override;

  void FlushForTesting();

 private:
  // SessionObserver:
  void OnActiveUserPrefServiceChanged(PrefService* pref_service) override;

  // TabletModeObserver:
  void OnTabletModeStarted() override;
  void OnTabletModeEnded() override;

  // WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanged() override;
  void OnWindowTreeHostReusedForDisplay(
      AshWindowTreeHost* window_tree_host,
      const display::Display& display) override;
  void OnWindowTreeHostsSwappedDisplays(AshWindowTreeHost* host1,
                                        AshWindowTreeHost* host2) override;

  // The shelf model shared by all shelf instances.
  ShelfModel model_;

  // Bindings for the ShelfController interface.
  mojo::BindingSet<mojom::ShelfController> bindings_;

  // True when applying changes from the remote ShelfModel owned by Chrome.
  // Changes to the local ShelfModel should not be reported during this time.
  bool applying_remote_shelf_model_changes_ = false;

  // Whether touchable context menus have been enabled for app icons on the
  // shelf.
  const bool is_touchable_app_context_menu_enabled_;

  ScopedObserver<message_center::MessageCenter,
                 message_center::MessageCenterObserver>
      message_center_observer_;

  // The set of shelf observers notified about state and model changes.
  mojo::AssociatedInterfacePtrSet<mojom::ShelfObserver> observers_;

  // Observes user profile prefs for the shelf.
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(ShelfController);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_CONTROLLER_H_
