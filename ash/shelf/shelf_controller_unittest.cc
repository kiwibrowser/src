// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_controller.h"

#include <string>

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/shelf_model.h"
#include "ash/public/cpp/shelf_model_observer.h"
#include "ash/public/cpp/shelf_prefs.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ui_base_features.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notifier_id.h"

namespace ash {
namespace {

Shelf* GetShelfForDisplay(int64_t display_id) {
  return Shell::GetRootWindowControllerWithDisplayId(display_id)->shelf();
}

void BuildAndSendNotification(message_center::MessageCenter* message_center,
                              const std::string& app_id,
                              const std::string& notification_id) {
  const message_center::NotifierId notifier_id(
      message_center::NotifierId::APPLICATION, app_id);
  std::unique_ptr<message_center::Notification> notification =
      std::make_unique<message_center::Notification>(
          message_center::NOTIFICATION_TYPE_SIMPLE, notification_id,
          base::ASCIIToUTF16("Test Web Notification"),
          base::ASCIIToUTF16("Notification message body."), gfx::Image(),
          base::ASCIIToUTF16("www.test.org"), GURL(), notifier_id,
          message_center::RichNotificationData(), nullptr /* delegate */);
  message_center->AddNotification(std::move(notification));
}

// A test implementation of the ShelfObserver mojo interface.
class TestShelfObserver : public mojom::ShelfObserver {
 public:
  TestShelfObserver() = default;
  ~TestShelfObserver() override = default;

  // mojom::ShelfObserver:
  void OnShelfItemAdded(int32_t, const ShelfItem& item) override {
    added_count_++;
    last_item_ = item;
  }
  void OnShelfItemRemoved(const ShelfID&) override { removed_count_++; }
  void OnShelfItemMoved(const ShelfID&, int32_t) override {}
  void OnShelfItemUpdated(const ShelfItem& item) override { last_item_ = item; }
  void OnShelfItemDelegateChanged(const ShelfID&,
                                  mojom::ShelfItemDelegatePtr) override {}

  size_t added_count() const { return added_count_; }
  size_t removed_count() const { return removed_count_; }
  const ShelfItem& last_item() const { return last_item_; }

 private:
  size_t added_count_ = 0;
  size_t removed_count_ = 0;
  ShelfItem last_item_;

  DISALLOW_COPY_AND_ASSIGN(TestShelfObserver);
};

using ShelfControllerTest = AshTestBase;

TEST_F(ShelfControllerTest, InitializesBackButtonAndAppListItemDelegate) {
  ShelfModel* model = Shell::Get()->shelf_controller()->model();
  EXPECT_EQ(2, model->item_count());
  EXPECT_EQ(kBackButtonId, model->items()[0].id.app_id);
  EXPECT_FALSE(model->GetShelfItemDelegate(ShelfID(kBackButtonId)));
  EXPECT_EQ(kAppListId, model->items()[1].id.app_id);
  EXPECT_TRUE(model->GetShelfItemDelegate(ShelfID(kAppListId)));
}

TEST_F(ShelfControllerTest, Shutdown) {
  // Simulate a display change occurring during shutdown (e.g. due to a screen
  // rotation animation being canceled).
  Shell::Get()->shelf_controller()->Shutdown();
  display_manager()->SetDisplayRotation(
      display::Screen::GetScreen()->GetPrimaryDisplay().id(),
      display::Display::ROTATE_90, display::Display::RotationSource::ACTIVE);
  // Ash does not crash during cleanup.
}

TEST_F(ShelfControllerTest, ShelfModelChangeSynchronization) {
  ShelfController* controller = Shell::Get()->shelf_controller();

  TestShelfObserver observer;
  mojom::ShelfObserverAssociatedPtr observer_ptr;
  mojo::AssociatedBinding<mojom::ShelfObserver> binding(
      &observer, mojo::MakeRequestAssociatedWithDedicatedPipe(&observer_ptr));
  controller->AddObserver(observer_ptr.PassInterface());
  base::RunLoop().RunUntilIdle();

  // The ShelfModel should be initialized with a two items, one for the back
  // button and one for the AppList. The observer is immediately notified of
  // existing shelf items.
  EXPECT_EQ(2, controller->model()->item_count());
  EXPECT_EQ(2u, observer.added_count());
  EXPECT_EQ(0u, observer.removed_count());

  // Add a ShelfModel item; |observer| should be notified.
  ShelfItem item;
  item.type = TYPE_PINNED_APP;
  item.id = ShelfID("foo");
  int index = controller->model()->Add(item);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(3, controller->model()->item_count());
  EXPECT_EQ(3u, observer.added_count());
  EXPECT_EQ(0u, observer.removed_count());

  // Remove a ShelfModel item; |observer| should be notified.
  controller->model()->RemoveItemAt(index);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, controller->model()->item_count());
  EXPECT_EQ(3u, observer.added_count());
  EXPECT_EQ(1u, observer.removed_count());

  // Simulate adding an item remotely; Ash should apply the change.
  // |observer| is not notified; see mojom::ShelfController for rationale.
  controller->AddShelfItem(index, item);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(3, controller->model()->item_count());
  EXPECT_EQ(3u, observer.added_count());
  EXPECT_EQ(1u, observer.removed_count());

  // Simulate removing an item remotely; Ash should apply the change.
  // |observer| is not notified; see mojom::ShelfController for rationale.
  controller->RemoveShelfItem(item.id);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, controller->model()->item_count());
  EXPECT_EQ(3u, observer.added_count());
  EXPECT_EQ(1u, observer.removed_count());
}

TEST_F(ShelfControllerTest, ShelfItemImageSynchronization) {
  ShelfController* controller = Shell::Get()->shelf_controller();

  TestShelfObserver observer;
  mojom::ShelfObserverAssociatedPtr observer_ptr;
  mojo::AssociatedBinding<mojom::ShelfObserver> binding(
      &observer, mojo::MakeRequestAssociatedWithDedicatedPipe(&observer_ptr));
  controller->AddObserver(observer_ptr.PassInterface());
  base::RunLoop().RunUntilIdle();

  // Create a ShelfItem struct with a valid image icon.
  ShelfItem item;
  item.type = TYPE_PINNED_APP;
  item.id = ShelfID("foo");
  item.image = gfx::test::CreateImageSkia(1, 1);

  // Observers are notifed of added items without images for efficiency.
  int index = controller->model()->Add(item);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(item.id, controller->model()->items()[index].id);
  EXPECT_FALSE(controller->model()->items()[index].image.isNull());
  EXPECT_EQ(item.id, observer.last_item().id);
  EXPECT_TRUE(observer.last_item().image.isNull());

  // Observers are notifed of updated items without images for efficiency.
  item.type = TYPE_APP;
  controller->model()->Set(index, item);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(item.type, controller->model()->items()[index].type);
  EXPECT_FALSE(controller->model()->items()[index].image.isNull());
  EXPECT_EQ(item.type, observer.last_item().type);
  EXPECT_TRUE(observer.last_item().image.isNull());

  // ShelfController should use images from remotely-added items.
  item.id = ShelfID("bar");
  controller->AddShelfItem(index, item);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(item.id, controller->model()->items()[index].id);
  EXPECT_FALSE(controller->model()->items()[index].image.isNull());

  // ShelfController should use images from remotely-updated items.
  item.image = gfx::test::CreateImageSkia(2, 2);
  controller->UpdateShelfItem(item);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(gfx::Size(2, 2), controller->model()->items()[index].image.size());

  // ShelfController should retain images when remote updates have no image.
  // Chrome will generally avoid image transport costs for unrelated updates.
  item.image = gfx::ImageSkia();
  controller->UpdateShelfItem(item);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(item.image.isNull());
  EXPECT_FALSE(controller->model()->items()[index].image.isNull());
}

class ShelfControllerTouchableContextMenuTest : public AshTestBase {
 public:
  ShelfControllerTouchableContextMenuTest() = default;
  ~ShelfControllerTouchableContextMenuTest() override = default;

  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        features::kTouchableAppContextMenu);
    AshTestBase::SetUp();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ShelfControllerTouchableContextMenuTest);
};

// Tests that the ShelfController keeps the ShelfModel updated on new
// notifications.
TEST_F(ShelfControllerTouchableContextMenuTest, HasNotificationBasic) {
  ShelfController* controller = Shell::Get()->shelf_controller();
  const std::string app_id("app_id");
  ShelfItem item;
  item.type = TYPE_APP;
  item.id = ShelfID(app_id);
  const int index = controller->model()->Add(item);
  EXPECT_FALSE(controller->model()->items()[index].has_notification);

  // Add a notification for |item|.
  message_center::MessageCenter* message_center =
      message_center::MessageCenter::Get();
  const std::string notification_id("notification_id");
  BuildAndSendNotification(message_center, app_id, notification_id);

  EXPECT_TRUE(controller->model()->items()[index].has_notification);

  // Remove the app and pin it, the notification should persist.
  controller->model()->RemoveItemAt(index);
  controller->model()->PinAppWithID(app_id);

  EXPECT_TRUE(controller->model()->items()[index].has_notification);

  message_center->RemoveNotification(notification_id, true);

  EXPECT_FALSE(controller->model()->items()[index].has_notification);
}

class ShelfControllerPrefsTest : public AshTestBase {
 public:
  ShelfControllerPrefsTest() = default;
  ~ShelfControllerPrefsTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShelfControllerPrefsTest);
};

// Ensure shelf settings are updated on preference changes.
TEST_F(ShelfControllerPrefsTest, ShelfRespectsPrefs) {
  Shelf* shelf = GetPrimaryShelf();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf->auto_hide_behavior());

  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  prefs->SetString(prefs::kShelfAlignmentLocal, "Left");
  prefs->SetString(prefs::kShelfAutoHideBehaviorLocal, "Always");

  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf->auto_hide_behavior());
}

// Ensure shelf settings are updated on per-display preference changes.
TEST_F(ShelfControllerPrefsTest, ShelfRespectsPerDisplayPrefs) {
  UpdateDisplay("1024x768,800x600");
  base::RunLoop().RunUntilIdle();
  const int64_t id1 = GetPrimaryDisplay().id();
  const int64_t id2 = GetSecondaryDisplay().id();
  Shelf* shelf1 = GetShelfForDisplay(id1);
  Shelf* shelf2 = GetShelfForDisplay(id2);

  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf1->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf2->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf1->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf2->auto_hide_behavior());

  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  SetShelfAlignmentPref(prefs, id1, SHELF_ALIGNMENT_LEFT);
  SetShelfAlignmentPref(prefs, id2, SHELF_ALIGNMENT_RIGHT);
  SetShelfAutoHideBehaviorPref(prefs, id1, SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  SetShelfAutoHideBehaviorPref(prefs, id2, SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);

  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf1->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_RIGHT, shelf2->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf1->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf2->auto_hide_behavior());
}

// Ensures that pre-Unified Mode per-display shelf settings don't prevent us
// from changing the shelf settings in unified mode.
TEST_F(ShelfControllerPrefsTest, ShelfRespectsPerDisplayPrefsUnified) {
  UpdateDisplay("1024x768,800x600");

  // Before enabling Unified Mode, set the shelf alignment for one of the two
  // displays, so that we have a per-display shelf alignment setting.
  ASSERT_FALSE(display_manager()->IsInUnifiedMode());
  const int64_t non_unified_primary_id = GetPrimaryDisplay().id();
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  Shelf* shelf = GetShelfForDisplay(non_unified_primary_id);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  SetShelfAlignmentPref(prefs, non_unified_primary_id, SHELF_ALIGNMENT_LEFT);
  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());

  // Switch to Unified Mode, and expect to be able to change the shelf
  // alignment.
  display_manager()->SetUnifiedDesktopEnabled(true);
  ASSERT_TRUE(display_manager()->IsInUnifiedMode());
  const int64_t unified_id = display::kUnifiedDisplayId;
  ASSERT_EQ(unified_id, GetPrimaryDisplay().id());

  shelf = GetShelfForDisplay(unified_id);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf->auto_hide_behavior());

  SetShelfAlignmentPref(prefs, unified_id, SHELF_ALIGNMENT_LEFT);
  SetShelfAutoHideBehaviorPref(prefs, unified_id,
                               SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);

  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf->auto_hide_behavior());

  SetShelfAlignmentPref(prefs, unified_id, SHELF_ALIGNMENT_RIGHT);
  EXPECT_EQ(SHELF_ALIGNMENT_RIGHT, shelf->alignment());
}

// Ensure shelf settings are correct after display swap, see crbug.com/748291
TEST_F(ShelfControllerPrefsTest, ShelfSettingsValidAfterDisplaySwap) {
  // Simulate adding an external display at the lock screen.
  GetSessionControllerClient()->RequestLockScreen();
  UpdateDisplay("1024x768,800x600");
  base::RunLoop().RunUntilIdle();
  const int64_t internal_display_id = GetPrimaryDisplay().id();
  const int64_t external_display_id = GetSecondaryDisplay().id();

  // The primary shelf should be on the internal display.
  EXPECT_EQ(GetPrimaryShelf(), GetShelfForDisplay(internal_display_id));
  EXPECT_NE(GetPrimaryShelf(), GetShelfForDisplay(external_display_id));

  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  // Check for the default shelf preferences.
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfAutoHideBehaviorPref(prefs, internal_display_id));
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfAutoHideBehaviorPref(prefs, external_display_id));
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM,
            GetShelfAlignmentPref(prefs, internal_display_id));
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM,
            GetShelfAlignmentPref(prefs, external_display_id));

  // Check the current state; shelves have locked alignments in the lock screen.
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(internal_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(external_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(internal_display_id)->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(external_display_id)->alignment());

  // Set some shelf prefs to differentiate the two shelves, check state.
  SetShelfAlignmentPref(prefs, internal_display_id, SHELF_ALIGNMENT_LEFT);
  SetShelfAlignmentPref(prefs, external_display_id, SHELF_ALIGNMENT_RIGHT);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(internal_display_id)->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(external_display_id)->alignment());

  SetShelfAutoHideBehaviorPref(prefs, external_display_id,
                               SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(internal_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS,
            GetShelfForDisplay(external_display_id)->auto_hide_behavior());

  // Simulate the external display becoming the primary display. The shelves are
  // swapped (each instance now has a different display id), check state.
  SwapPrimaryDisplay();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(internal_display_id)->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED,
            GetShelfForDisplay(external_display_id)->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(internal_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS,
            GetShelfForDisplay(external_display_id)->auto_hide_behavior());

  // After screen unlock the shelves should have the expected alignment values.
  GetSessionControllerClient()->UnlockScreen();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(SHELF_ALIGNMENT_LEFT,
            GetShelfForDisplay(internal_display_id)->alignment());
  EXPECT_EQ(SHELF_ALIGNMENT_RIGHT,
            GetShelfForDisplay(external_display_id)->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER,
            GetShelfForDisplay(internal_display_id)->auto_hide_behavior());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS,
            GetShelfForDisplay(external_display_id)->auto_hide_behavior());
}

TEST_F(ShelfControllerPrefsTest, ShelfSettingsInTabletMode) {
  Shelf* shelf = GetPrimaryShelf();
  PrefService* prefs =
      Shell::Get()->session_controller()->GetLastActiveUserPrefService();
  SetShelfAlignmentPref(prefs, GetPrimaryDisplay().id(), SHELF_ALIGNMENT_LEFT);
  SetShelfAutoHideBehaviorPref(prefs, GetPrimaryDisplay().id(),
                               SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  ASSERT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());
  ASSERT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf->auto_hide_behavior());

  // Verify after entering tablet mode, the shelf alignment is bottom and the
  // auto hide behavior is never.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf->auto_hide_behavior());

  // Verify that screen rotation does not change alignment or auto-hide.
  display_manager()->SetDisplayRotation(
      display::Screen::GetScreen()->GetPrimaryDisplay().id(),
      display::Display::ROTATE_90, display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf->auto_hide_behavior());

  // Verify after exiting tablet mode, the shelf alignment and auto hide
  // behavior get their stored pref values.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf->auto_hide_behavior());
}

using ShelfControllerAppModeTest = NoSessionAshTestBase;

// Tests that shelf auto hide behavior is always hidden in app mode.
TEST_F(ShelfControllerAppModeTest, AutoHideBehavior) {
  SimulateKioskMode(user_manager::USER_TYPE_KIOSK_APP);

  Shelf* shelf = GetPrimaryShelf();
  EXPECT_EQ(SHELF_AUTO_HIDE_ALWAYS_HIDDEN, shelf->auto_hide_behavior());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_EQ(SHELF_AUTO_HIDE_ALWAYS_HIDDEN, shelf->auto_hide_behavior());

  display_manager()->SetDisplayRotation(
      display::Screen::GetScreen()->GetPrimaryDisplay().id(),
      display::Display::ROTATE_90, display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(SHELF_AUTO_HIDE_ALWAYS_HIDDEN, shelf->auto_hide_behavior());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_EQ(SHELF_AUTO_HIDE_ALWAYS_HIDDEN, shelf->auto_hide_behavior());
}

}  // namespace
}  // namespace ash
