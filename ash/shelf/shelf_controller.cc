// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_controller.h"

#include <memory>

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/remote_shelf_item_delegate.h"
#include "ash/public/cpp/shelf_prefs.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/app_list_shelf_item_delegate.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/message_center/arc/arc_notification_constants.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/auto_reset.h"
#include "base/strings/utf_string_conversions.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/ui_base_features.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/message_center/message_center.h"

namespace ash {

namespace {

// Returns the Shelf instance for the display with the given |display_id|.
Shelf* GetShelfForDisplay(int64_t display_id) {
  // The controller may be null for invalid ids or for displays being removed.
  RootWindowController* root_window_controller =
      Shell::GetRootWindowControllerWithDisplayId(display_id);
  return root_window_controller ? root_window_controller->shelf() : nullptr;
}

// Set each Shelf's auto-hide behavior from the per-display pref.
void SetShelfAutoHideFromPrefs() {
  // TODO(jamescook): The session state check should not be necessary, but
  // otherwise this wrongly tries to set the alignment on a secondary display
  // during login before the ShelfLockingManager is created.
  SessionController* session_controller = Shell::Get()->session_controller();
  PrefService* prefs = session_controller->GetLastActiveUserPrefService();
  if (!prefs || !session_controller->IsActiveUserSessionStarted())
    return;

  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    auto value = GetShelfAutoHideBehaviorPref(prefs, display.id());
    // Don't show the shelf in app mode.
    if (session_controller->IsRunningInAppMode())
      value = SHELF_AUTO_HIDE_ALWAYS_HIDDEN;
    Shelf* shelf = GetShelfForDisplay(display.id());
    if (shelf)
      shelf->SetAutoHideBehavior(value);
  }
}

// Set each Shelf's alignment from the per-display pref.
void SetShelfAlignmentFromPrefs() {
  // TODO(jamescook): The session state check should not be necessary, but
  // otherwise this wrongly tries to set the alignment on a secondary display
  // during login before the ShelfLockingManager is created.
  SessionController* session_controller = Shell::Get()->session_controller();
  PrefService* prefs = session_controller->GetLastActiveUserPrefService();
  if (!prefs || !session_controller->IsActiveUserSessionStarted())
    return;

  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    auto value = GetShelfAlignmentPref(prefs, display.id());
    Shelf* shelf = GetShelfForDisplay(display.id());
    if (shelf)
      shelf->SetAlignment(value);
  }
}

void UpdateShelfVisibility() {
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    Shelf* shelf = GetShelfForDisplay(display.id());
    if (shelf)
      shelf->UpdateVisibilityState();
  }
}

// Set each Shelf's auto-hide behavior and alignment from the per-display prefs.
void SetShelfBehaviorsFromPrefs() {
  // The shelf should always be bottom-aligned and not hidden in tablet mode;
  // alignment and auto-hide are assigned from prefs when tablet mode is exited.
  if (Shell::Get()
          ->tablet_mode_controller()
          ->IsTabletModeWindowManagerEnabled()) {
    return;
  }

  SetShelfAutoHideFromPrefs();
  SetShelfAlignmentFromPrefs();
}

}  // namespace

ShelfController::ShelfController()
    : is_touchable_app_context_menu_enabled_(
          features::IsTouchableAppContextMenuEnabled()),
      message_center_observer_(this) {
  // Set the delegate and title string for the back button.
  model_.SetShelfItemDelegate(ShelfID(kBackButtonId), nullptr);
  DCHECK_EQ(0, model_.ItemIndexByID(ShelfID(kBackButtonId)));
  ShelfItem back_item = model_.items()[0];
  back_item.title = l10n_util::GetStringUTF16(IDS_ASH_SHELF_BACK_BUTTON_TITLE);
  model_.Set(0, back_item);

  // Set the delegate and title string for the app list item.
  model_.SetShelfItemDelegate(ShelfID(kAppListId),
                              std::make_unique<AppListShelfItemDelegate>());
  DCHECK_EQ(1, model_.ItemIndexByID(ShelfID(kAppListId)));
  ShelfItem launcher_item = model_.items()[1];
  launcher_item.title =
      l10n_util::GetStringUTF16(IDS_ASH_SHELF_APP_LIST_LAUNCHER_TITLE);
  model_.Set(1, launcher_item);

  model_.AddObserver(this);
  Shell::Get()->session_controller()->AddObserver(this);
  Shell::Get()->tablet_mode_controller()->AddObserver(this);
  Shell::Get()->window_tree_host_manager()->AddObserver(this);
  if (is_touchable_app_context_menu_enabled_)
    message_center_observer_.Add(message_center::MessageCenter::Get());
}

ShelfController::~ShelfController() {
  model_.RemoveObserver(this);
  model_.DestroyItemDelegates();
}

void ShelfController::Shutdown() {
  Shell::Get()->window_tree_host_manager()->RemoveObserver(this);
  Shell::Get()->tablet_mode_controller()->RemoveObserver(this);
  Shell::Get()->session_controller()->RemoveObserver(this);
}

// static
void ShelfController::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  // These prefs are public for ChromeLauncherController's OnIsSyncingChanged.
  // See the pref names definitions for explanations of the synced, local, and
  // per-display behaviors.
  registry->RegisterStringPref(
      prefs::kShelfAutoHideBehavior, kShelfAutoHideBehaviorNever,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF | PrefRegistry::PUBLIC);
  registry->RegisterStringPref(prefs::kShelfAutoHideBehaviorLocal,
                               std::string(), PrefRegistry::PUBLIC);
  registry->RegisterStringPref(
      prefs::kShelfAlignment, kShelfAlignmentBottom,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF | PrefRegistry::PUBLIC);
  registry->RegisterStringPref(prefs::kShelfAlignmentLocal, std::string(),
                               PrefRegistry::PUBLIC);
  registry->RegisterDictionaryPref(prefs::kShelfPreferences,
                                   PrefRegistry::PUBLIC);
}

void ShelfController::BindRequest(mojom::ShelfControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ShelfController::AddObserver(
    mojom::ShelfObserverAssociatedPtrInfo observer) {
  mojom::ShelfObserverAssociatedPtr observer_ptr;
  observer_ptr.Bind(std::move(observer));

  // Synchronize two ShelfModel instances, one each owned by Ash and Chrome.
  // Notify Chrome of existing ShelfModel items and delegates created by Ash.
  for (int i = 0; i < model_.item_count(); ++i) {
    ShelfItem item = model_.items()[i];
    ShelfItemDelegate* delegate = model_.GetShelfItemDelegate(item.id);
    // Notify observers of the delegate before the items themselves; Chrome
    // creates default delegates if none exist, breaking ShelfWindowWatcher.
    if (delegate) {
      observer_ptr->OnShelfItemDelegateChanged(
          item.id, delegate->CreateInterfacePtrAndBind());
    }
    // Pass null images to avoid transport costs; clients don't use images.
    item.image = gfx::ImageSkia();
    observer_ptr->OnShelfItemAdded(i, item);
  }

  observers_.AddPtr(std::move(observer_ptr));
}

void ShelfController::AddShelfItem(int32_t index, const ShelfItem& item) {
  DCHECK(!applying_remote_shelf_model_changes_) << " Unexpected model change";
  index = index < 0 ? model_.item_count() : index;
  DCHECK_GT(index, 0) << " Items can not precede the AppList";
  DCHECK_LE(index, model_.item_count()) << " Index out of bounds";
  index = std::min(std::max(index, 1), model_.item_count());
  base::AutoReset<bool> reset(&applying_remote_shelf_model_changes_, true);
  model_.AddAt(index, item);
}

void ShelfController::RemoveShelfItem(const ShelfID& id) {
  DCHECK(!applying_remote_shelf_model_changes_) << " Unexpected model change";
  const int index = model_.ItemIndexByID(id);
  DCHECK_GE(index, 0) << " No item found with the id: " << id;
  DCHECK_NE(index, 0) << " The AppList shelf item cannot be removed";
  if (index <= 0)
    return;
  base::AutoReset<bool> reset(&applying_remote_shelf_model_changes_, true);
  model_.RemoveItemAt(index);
}

void ShelfController::MoveShelfItem(const ShelfID& id, int32_t index) {
  DCHECK(!applying_remote_shelf_model_changes_) << " Unexpected model change";
  const int current_index = model_.ItemIndexByID(id);
  DCHECK_GE(current_index, 0) << " No item found with the id: " << id;
  DCHECK_NE(current_index, 0) << " The AppList shelf item cannot be moved";
  if (current_index <= 0)
    return;
  DCHECK_GT(index, 0) << " Items can not precede the AppList";
  DCHECK_LT(index, model_.item_count()) << " Index out of bounds";
  index = std::min(std::max(index, 1), model_.item_count() - 1);
  if (current_index == index) {
    DVLOG(1) << "The item is already at the given index (" << index << "). "
             << "This happens when syncing a ShelfModel weight reordering.";
    return;
  }
  base::AutoReset<bool> reset(&applying_remote_shelf_model_changes_, true);
  model_.Move(current_index, index);
}

void ShelfController::UpdateShelfItem(const ShelfItem& item) {
  DCHECK(!applying_remote_shelf_model_changes_) << " Unexpected model change";
  const int index = model_.ItemIndexByID(item.id);
  DCHECK_GE(index, 0) << " No item found with the id: " << item.id;
  if (index < 0)
    return;
  base::AutoReset<bool> reset(&applying_remote_shelf_model_changes_, true);

  // Keep any existing image if the item was sent without one for efficiency.
  ShelfItem new_item = item;
  if (item.image.isNull())
    new_item.image = model_.items()[index].image;
  model_.Set(index, new_item);
}

void ShelfController::SetShelfItemDelegate(
    const ShelfID& id,
    mojom::ShelfItemDelegatePtr delegate) {
  DCHECK(!applying_remote_shelf_model_changes_) << " Unexpected model change";
  base::AutoReset<bool> reset(&applying_remote_shelf_model_changes_, true);
  if (delegate.is_bound())
    model_.SetShelfItemDelegate(
        id, std::make_unique<RemoteShelfItemDelegate>(id, std::move(delegate)));
  else
    model_.SetShelfItemDelegate(id, nullptr);
}

void ShelfController::ShelfItemAdded(int index) {
  if (applying_remote_shelf_model_changes_)
    return;

  // Pass null images to avoid transport costs; clients don't use images.
  ShelfItem item = model_.items()[index];
  item.image = gfx::ImageSkia();
  observers_.ForAllPtrs([index, item](mojom::ShelfObserver* observer) {
    observer->OnShelfItemAdded(index, item);
  });
}

void ShelfController::ShelfItemRemoved(int index, const ShelfItem& old_item) {
  if (applying_remote_shelf_model_changes_)
    return;

  observers_.ForAllPtrs([old_item](mojom::ShelfObserver* observer) {
    observer->OnShelfItemRemoved(old_item.id);
  });
}

void ShelfController::ShelfItemMoved(int start_index, int target_index) {
  if (applying_remote_shelf_model_changes_)
    return;

  const ShelfItem& item = model_.items()[target_index];
  observers_.ForAllPtrs([item, target_index](mojom::ShelfObserver* observer) {
    observer->OnShelfItemMoved(item.id, target_index);
  });
}

void ShelfController::ShelfItemChanged(int index, const ShelfItem& old_item) {
  if (applying_remote_shelf_model_changes_)
    return;

  // Pass null images to avoid transport costs; clients don't use images.
  ShelfItem item = model_.items()[index];
  item.image = gfx::ImageSkia();
  observers_.ForAllPtrs([item](mojom::ShelfObserver* observer) {
    observer->OnShelfItemUpdated(item);
  });
}

void ShelfController::ShelfItemDelegateChanged(const ShelfID& id,
                                               ShelfItemDelegate* old_delegate,
                                               ShelfItemDelegate* delegate) {
  if (applying_remote_shelf_model_changes_)
    return;

  observers_.ForAllPtrs([id, delegate](mojom::ShelfObserver* observer) {
    observer->OnShelfItemDelegateChanged(
        id, delegate ? delegate->CreateInterfacePtrAndBind()
                     : mojom::ShelfItemDelegatePtr());
  });
}

void ShelfController::FlushForTesting() {
  bindings_.FlushForTesting();
}

void ShelfController::OnActiveUserPrefServiceChanged(
    PrefService* pref_service) {
  SetShelfBehaviorsFromPrefs();
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(pref_service);
  pref_change_registrar_->Add(prefs::kShelfAlignmentLocal,
                              base::BindRepeating(&SetShelfAlignmentFromPrefs));
  pref_change_registrar_->Add(prefs::kShelfAutoHideBehaviorLocal,
                              base::BindRepeating(&SetShelfAutoHideFromPrefs));
  pref_change_registrar_->Add(prefs::kShelfPreferences,
                              base::BindRepeating(&SetShelfBehaviorsFromPrefs));
}

void ShelfController::OnTabletModeStarted() {
  // Do nothing when running in app mode.
  if (Shell::Get()->session_controller()->IsRunningInAppMode())
    return;

  // Force the shelf to be visible and to be bottom aligned in tablet mode; the
  // prefs are restored on exit.
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    Shelf* shelf = GetShelfForDisplay(display.id());
    if (shelf) {
      // Only animate into tablet mode if the shelf alignment will not change.
      if (shelf->IsHorizontalAlignment())
        shelf->set_is_tablet_mode_animation_running(true);
      shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_NEVER);
      shelf->SetAlignment(SHELF_ALIGNMENT_BOTTOM);
    }
  }
}

void ShelfController::OnTabletModeEnded() {
  // Do nothing when running in app mode.
  if (Shell::Get()->session_controller()->IsRunningInAppMode())
    return;

  SetShelfBehaviorsFromPrefs();
  // Only animate out of tablet mode if the shelf alignment will not change.
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    Shelf* shelf = GetShelfForDisplay(display.id());
    if (shelf && shelf->IsHorizontalAlignment())
      shelf->set_is_tablet_mode_animation_running(true);
  }
}

void ShelfController::OnDisplayConfigurationChanged() {
  // Set/init the shelf behaviors from preferences, in case a display was added.
  SetShelfBehaviorsFromPrefs();

  // Update shelf visibility to adapt to display changes. For instance shelf
  // should be hidden on secondary display during inactive session states.
  UpdateShelfVisibility();
}

void ShelfController::OnWindowTreeHostReusedForDisplay(
    AshWindowTreeHost* window_tree_host,
    const display::Display& display) {
  // See comment in OnWindowTreeHostsSwappedDisplays().
  SetShelfBehaviorsFromPrefs();

  // Update shelf visibility to adapt to display changes. For instance shelf
  // should be hidden on secondary display during inactive session states.
  UpdateShelfVisibility();
}

void ShelfController::OnWindowTreeHostsSwappedDisplays(
    AshWindowTreeHost* host1,
    AshWindowTreeHost* host2) {
  // The display ids for existing shelf instances may have changed, so update
  // the alignment and auto-hide state from prefs. See http://crbug.com/748291
  SetShelfBehaviorsFromPrefs();

  // Update shelf visibility to adapt to display changes. For instance shelf
  // should be hidden on secondary display during inactive session states.
  UpdateShelfVisibility();
}

void ShelfController::OnNotificationAdded(const std::string& notification_id) {
  if (!is_touchable_app_context_menu_enabled_)
    return;

  message_center::Notification* notification =
      message_center::MessageCenter::Get()->FindVisibleNotificationById(
          notification_id);

  if (!notification)
    return;

  // Skip this if the notification shouldn't badge an app.
  if (notification->notifier_id().type !=
          message_center::NotifierId::APPLICATION &&
      notification->notifier_id().type !=
          message_center::NotifierId::ARC_APPLICATION) {
    return;
  }

  // Skip this if the notification doesn't have a valid app id.
  if (notification->notifier_id().id == kDefaultArcNotifierId)
    return;

  model_.AddNotificationRecord(notification->notifier_id().id, notification_id);
}

void ShelfController::OnNotificationRemoved(const std::string& notification_id,
                                            bool by_user) {
  if (!is_touchable_app_context_menu_enabled_)
    return;

  model_.RemoveNotificationRecord(notification_id);
}

}  // namespace ash
