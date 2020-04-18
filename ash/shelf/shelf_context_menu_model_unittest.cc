// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_context_menu_model.h"

#include "ash/public/cpp/config.h"
#include "ash/public/cpp/shelf_item_delegate.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test_shell_delegate.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/strings/utf_string_conversions.h"
#include "chromeos/login/scoped_test_public_session_login_state.h"
#include "ui/display/display.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

using CommandId = ShelfContextMenuModel::CommandId;
using MenuItemList = std::vector<mojom::MenuItemPtr>;

class ShelfContextMenuModelTest : public AshTestBase {
 public:
  ShelfContextMenuModelTest() = default;
  ~ShelfContextMenuModelTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();
    TestSessionControllerClient* session = GetSessionControllerClient();
    session->AddUserSession("user1@test.com");
    session->SetSessionState(session_manager::SessionState::ACTIVE);
    session->SwitchActiveUser(AccountId::FromUserEmail("user1@test.com"));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ShelfContextMenuModelTest);
};

// A test wallpaper controller client class.
class TestWallpaperControllerClient : public mojom::WallpaperControllerClient {
 public:
  TestWallpaperControllerClient() : binding_(this) {}
  ~TestWallpaperControllerClient() override = default;

  size_t open_count() const { return open_count_; }

  mojom::WallpaperControllerClientPtr CreateInterfacePtr() {
    mojom::WallpaperControllerClientPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // mojom::WallpaperControllerClient:
  void OpenWallpaperPicker() override { open_count_++; }
  void OnReadyToSetWallpaper() override {}
  void OnFirstWallpaperAnimationFinished() override {}

 private:
  size_t open_count_ = 0;
  mojo::Binding<mojom::WallpaperControllerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestWallpaperControllerClient);
};

// A test shelf item delegate that records the commands sent for execution.
class TestShelfItemDelegate : public ShelfItemDelegate {
 public:
  TestShelfItemDelegate() : ShelfItemDelegate(ShelfID()) {}
  ~TestShelfItemDelegate() override = default;

  int last_executed_command() const { return last_executed_command_; }

  // ShelfItemDelegate:
  void ItemSelected(std::unique_ptr<ui::Event> event,
                    int64_t display_id,
                    ShelfLaunchSource source,
                    ItemSelectedCallback callback) override {}
  void ExecuteCommand(bool from_context_menu,
                      int64_t command_id,
                      int32_t event_flags,
                      int64_t display_id) override {
    ASSERT_TRUE(from_context_menu);
    last_executed_command_ = command_id;
  }
  void Close() override {}

 private:
  int last_executed_command_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestShelfItemDelegate);
};

// Tests the default items in a shelf context menu.
TEST_F(ShelfContextMenuModelTest, Basic) {
  ShelfContextMenuModel menu(MenuItemList(), nullptr, GetPrimaryDisplay().id());

  ASSERT_EQ(3, menu.GetItemCount());
  EXPECT_EQ(CommandId::MENU_AUTO_HIDE, menu.GetCommandIdAt(0));
  EXPECT_EQ(CommandId::MENU_ALIGNMENT_MENU, menu.GetCommandIdAt(1));
  EXPECT_EQ(CommandId::MENU_CHANGE_WALLPAPER, menu.GetCommandIdAt(2));
  for (int i = 0; i < menu.GetItemCount(); ++i) {
    EXPECT_TRUE(menu.IsEnabledAt(i));
    EXPECT_TRUE(menu.IsVisibleAt(i));
  }

  // Check the alignment submenu.
  EXPECT_EQ(ui::MenuModel::TYPE_SUBMENU, menu.GetTypeAt(1));
  ui::MenuModel* submenu = menu.GetSubmenuModelAt(1);
  ASSERT_TRUE(submenu);
  ASSERT_EQ(3, submenu->GetItemCount());
  EXPECT_EQ(CommandId::MENU_ALIGNMENT_LEFT, submenu->GetCommandIdAt(0));
  EXPECT_EQ(CommandId::MENU_ALIGNMENT_BOTTOM, submenu->GetCommandIdAt(1));
  EXPECT_EQ(CommandId::MENU_ALIGNMENT_RIGHT, submenu->GetCommandIdAt(2));
}

// Test invocation of the default menu items.
TEST_F(ShelfContextMenuModelTest, Invocation) {
  int64_t primary_id = GetPrimaryDisplay().id();
  Shelf* shelf = GetPrimaryShelf();

  // Check the shelf auto-hide behavior and menu interaction.
  ShelfContextMenuModel menu1(MenuItemList(), nullptr, primary_id);
  EXPECT_FALSE(menu1.IsItemCheckedAt(0));
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_NEVER, shelf->auto_hide_behavior());
  menu1.ActivatedAt(0);
  EXPECT_EQ(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS, shelf->auto_hide_behavior());

  // This menu shows auto-hide enabled; check alignment and menu interaction.
  ShelfContextMenuModel menu2(MenuItemList(), nullptr, primary_id);
  EXPECT_TRUE(menu2.IsItemCheckedAt(0));
  ui::MenuModel* submenu = menu2.GetSubmenuModelAt(1);
  EXPECT_TRUE(submenu->IsItemCheckedAt(1));
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  submenu->ActivatedAt(0);
  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());

  // This menu shows left alignment; check wallpaper item interaction.
  ShelfContextMenuModel menu3(MenuItemList(), nullptr, primary_id);
  submenu = menu3.GetSubmenuModelAt(1);
  EXPECT_TRUE(submenu->IsItemCheckedAt(0));
  TestWallpaperControllerClient client;
  Shell::Get()->wallpaper_controller()->SetClientForTesting(
      client.CreateInterfacePtr());
  EXPECT_EQ(0u, client.open_count());
  menu3.ActivatedAt(2);
  Shell::Get()->wallpaper_controller()->FlushForTesting();
  EXPECT_EQ(1u, client.open_count());
}

// Tests the prepending of custom items in a shelf context menu.
TEST_F(ShelfContextMenuModelTest, CustomItems) {
  // Make a list of custom items with a variety of values.
  MenuItemList items;
  mojom::MenuItemPtr item(mojom::MenuItem::New());
  item->type = ui::MenuModel::TYPE_COMMAND;
  item->command_id = 123;
  item->label = base::ASCIIToUTF16("item");
  item->enabled = true;
  items.push_back(std::move(item));
  mojom::MenuItemPtr separator(mojom::MenuItem::New());
  separator->type = ui::MenuModel::TYPE_SEPARATOR;
  items.push_back(std::move(separator));
  mojom::MenuItemPtr check(mojom::MenuItem::New());
  check->type = ui::MenuModel::TYPE_CHECK;
  check->command_id = 999;
  check->label = base::ASCIIToUTF16("check");
  check->enabled = true;
  check->checked = false;
  items.push_back(std::move(check));
  mojom::MenuItemPtr radio(mojom::MenuItem::New());
  radio->type = ui::MenuModel::TYPE_RADIO;
  radio->command_id = 1337;
  radio->label = base::ASCIIToUTF16("radio");
  radio->enabled = false;
  radio->checked = true;
  items.push_back(std::move(radio));

  // Ensure the menu model's prepended contents match the items above.
  TestShelfItemDelegate delegate;
  ShelfContextMenuModel menu(std::move(items), &delegate,
                             GetPrimaryDisplay().id());
  ASSERT_EQ(7, menu.GetItemCount());

  EXPECT_EQ(ui::MenuModel::TYPE_COMMAND, menu.GetTypeAt(0));
  EXPECT_EQ(123, menu.GetCommandIdAt(0));
  EXPECT_EQ(base::ASCIIToUTF16("item"), menu.GetLabelAt(0));
  EXPECT_TRUE(menu.IsEnabledAt(0));

  EXPECT_EQ(ui::MenuModel::TYPE_SEPARATOR, menu.GetTypeAt(1));

  EXPECT_EQ(ui::MenuModel::TYPE_CHECK, menu.GetTypeAt(2));
  EXPECT_EQ(999, menu.GetCommandIdAt(2));
  EXPECT_EQ(base::ASCIIToUTF16("check"), menu.GetLabelAt(2));
  EXPECT_TRUE(menu.IsEnabledAt(2));
  EXPECT_FALSE(menu.IsItemCheckedAt(2));

  EXPECT_EQ(ui::MenuModel::TYPE_RADIO, menu.GetTypeAt(3));
  EXPECT_EQ(1337, menu.GetCommandIdAt(3));
  EXPECT_EQ(base::ASCIIToUTF16("radio"), menu.GetLabelAt(3));
  EXPECT_FALSE(menu.IsEnabledAt(3));
  EXPECT_TRUE(menu.IsItemCheckedAt(3));

  // The default contents should appear at the bottom.
  EXPECT_EQ(CommandId::MENU_AUTO_HIDE, menu.GetCommandIdAt(4));
  EXPECT_EQ(CommandId::MENU_ALIGNMENT_MENU, menu.GetCommandIdAt(5));
  EXPECT_EQ(CommandId::MENU_CHANGE_WALLPAPER, menu.GetCommandIdAt(6));

  // Invoking a custom item should execute the command id on the delegate.
  menu.ActivatedAt(2);
  EXPECT_EQ(999, delegate.last_executed_command());
}

// Tests the prepending of a custom submenu in a shelf context menu.
TEST_F(ShelfContextMenuModelTest, CustomSubmenu) {
  // Make a list of custom items that includes a submenu.
  MenuItemList items;
  mojom::MenuItemPtr submenu_item(mojom::MenuItem::New());
  submenu_item->type = ui::MenuModel::TYPE_SUBMENU;
  MenuItemList submenu_items;
  submenu_items.push_back(mojom::MenuItem::New());
  submenu_items.push_back(mojom::MenuItem::New());
  submenu_item->submenu = std::move(submenu_items);
  items.push_back(std::move(submenu_item));

  // Ensure the menu model's prepended contents match the items above.
  ShelfContextMenuModel menu(std::move(items), nullptr,
                             GetPrimaryDisplay().id());
  ASSERT_EQ(4, menu.GetItemCount());
  EXPECT_EQ(ui::MenuModel::TYPE_SUBMENU, menu.GetTypeAt(0));
  ui::MenuModel* submenu = menu.GetSubmenuModelAt(0);
  EXPECT_EQ(2, submenu->GetItemCount());

  // The default contents should appear at the bottom.
  EXPECT_EQ(CommandId::MENU_AUTO_HIDE, menu.GetCommandIdAt(1));
  EXPECT_EQ(CommandId::MENU_ALIGNMENT_MENU, menu.GetCommandIdAt(2));
  EXPECT_EQ(CommandId::MENU_CHANGE_WALLPAPER, menu.GetCommandIdAt(3));
}

// Tests fullscreen's per-display removal of "Autohide shelf": crbug.com/496681
TEST_F(ShelfContextMenuModelTest, AutohideShelfOptionOnExternalDisplay) {
  UpdateDisplay("940x550,940x550");
  int64_t primary_id = GetPrimaryDisplay().id();
  int64_t secondary_id = GetSecondaryDisplay().id();

  // Create a normal window on the primary display.
  std::unique_ptr<views::Widget> widget = CreateTestWidget();
  widget->Show();
  widget->SetFullscreen(true);

  ShelfContextMenuModel primary_menu(MenuItemList(), nullptr, primary_id);
  ShelfContextMenuModel secondary_menu(MenuItemList(), nullptr, secondary_id);
  EXPECT_EQ(-1, primary_menu.GetIndexOfCommandId(CommandId::MENU_AUTO_HIDE));
  EXPECT_NE(-1, secondary_menu.GetIndexOfCommandId(CommandId::MENU_AUTO_HIDE));
}

TEST_F(ShelfContextMenuModelTest, DisableAutoHideOptionOnTabletMode) {
  TabletModeController* tablet_mode_controller =
      Shell::Get()->tablet_mode_controller();
  int64_t primary_id = GetPrimaryDisplay().id();

  // Tests that in tablet mode, shelf auto-hide option is disabled.
  tablet_mode_controller->EnableTabletModeWindowManager(true);
  ShelfContextMenuModel menu1(MenuItemList(), nullptr, primary_id);
  ASSERT_EQ(CommandId::MENU_AUTO_HIDE, menu1.GetCommandIdAt(0));
  EXPECT_FALSE(menu1.IsEnabledAt(0));
  EXPECT_TRUE(menu1.IsVisibleAt(0));

  // Tests that exiting tablet mode reenables the auto-hide context menu item.
  tablet_mode_controller->EnableTabletModeWindowManager(false);
  ShelfContextMenuModel menu2(MenuItemList(), nullptr, primary_id);
  ASSERT_EQ(CommandId::MENU_AUTO_HIDE, menu2.GetCommandIdAt(0));
  EXPECT_TRUE(menu2.IsEnabledAt(0));
  EXPECT_TRUE(menu2.IsVisibleAt(0));
}

TEST_F(ShelfContextMenuModelTest, DisableAlignmentMenuOnTabletMode) {
  TabletModeController* tablet_mode_controller =
      Shell::Get()->tablet_mode_controller();
  int64_t primary_id = GetPrimaryDisplay().id();

  // Tests that in tablet mode, shelf alignment menu is disabled.
  tablet_mode_controller->EnableTabletModeWindowManager(true);
  ShelfContextMenuModel menu1(MenuItemList(), nullptr, primary_id);
  ASSERT_EQ(CommandId::MENU_ALIGNMENT_MENU, menu1.GetCommandIdAt(1));
  EXPECT_FALSE(menu1.IsEnabledAt(1));
  EXPECT_TRUE(menu1.IsVisibleAt(1));

  // Tests that exiting tablet mode reenables the shelf alignment menu.
  tablet_mode_controller->EnableTabletModeWindowManager(false);
  ShelfContextMenuModel menu2(MenuItemList(), nullptr, primary_id);
  ASSERT_EQ(CommandId::MENU_ALIGNMENT_MENU, menu2.GetCommandIdAt(1));
  EXPECT_TRUE(menu2.IsEnabledAt(1));
  EXPECT_TRUE(menu2.IsVisibleAt(1));
}

TEST_F(ShelfContextMenuModelTest, CommandIdsMatchEnumsForHistograms) {
  // Tests that CommandId enums are not changed as the values are used in
  // histograms.
  EXPECT_EQ(500, ShelfContextMenuModel::MENU_AUTO_HIDE);
  EXPECT_EQ(501, ShelfContextMenuModel::MENU_ALIGNMENT_MENU);
  EXPECT_EQ(502, ShelfContextMenuModel::MENU_ALIGNMENT_LEFT);
  EXPECT_EQ(503, ShelfContextMenuModel::MENU_ALIGNMENT_RIGHT);
  EXPECT_EQ(504, ShelfContextMenuModel::MENU_ALIGNMENT_BOTTOM);
  EXPECT_EQ(505, ShelfContextMenuModel::MENU_CHANGE_WALLPAPER);
}

TEST_F(ShelfContextMenuModelTest, ShelfContextMenuOptions) {
  // Tests that there are exactly 3 shelf context menu options. If you're adding
  // a context menu option ensure that you have added the enum to
  // tools/metrics/enums.xml and that you haven't modified the order of the
  // existing enums.
  ShelfContextMenuModel menu(MenuItemList(), nullptr, GetPrimaryDisplay().id());
  EXPECT_EQ(3, menu.GetItemCount());
}

}  // namespace
}  // namespace ash
