// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/menu_utils.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/models/menu_separator_types.h"
#include "ui/base/models/simple_menu_model.h"

using base::ASCIIToUTF16;

namespace ash {
namespace menu_utils {

using MenuModelList = std::vector<std::unique_ptr<ui::SimpleMenuModel>>;

class MenuUtilsTest : public testing::Test,
                      public ui::SimpleMenuModel::Delegate {
 public:
  MenuUtilsTest() {}
  ~MenuUtilsTest() override {}

  // testing::Test overrides:
  void SetUp() override {
    menu_model_ = std::make_unique<ui::SimpleMenuModel>(this);
  }

 protected:
  ui::SimpleMenuModel* root_menu() { return menu_model_.get(); }
  MenuModelList* submenus() { return &submenu_models_; }

  ui::SimpleMenuModel* CreateSubmenu() {
    std::unique_ptr<ui::SimpleMenuModel> submenu =
        std::make_unique<ui::SimpleMenuModel>(this);
    ui::SimpleMenuModel* submenu_ptr = submenu.get();
    submenu_models_.push_back(std::move(submenu));
    return submenu_ptr;
  }

  bool IsCommandIdChecked(int command_id) const override {
    // Assume that we have a checked item in every 3 items.
    return command_id % 3 == 0;
  }

  bool IsCommandIdEnabled(int command_id) const override {
    // Assume that we have a enabled item in every 4 items.
    return command_id % 4 == 0;
  }

  void ExecuteCommand(int command_id, int event_flags) override {}

  void CheckMenuItemsMatched(const MenuItemList& mojo_menu,
                             const ui::MenuModel* menu) {
    EXPECT_EQ(mojo_menu.size(), static_cast<size_t>(menu->GetItemCount()));
    for (size_t i = 0; i < mojo_menu.size(); i++) {
      VLOG(1) << "Checking item at " << i;
      mojom::MenuItem* mojo_item = mojo_menu[i].get();
      EXPECT_EQ(mojo_item->type, menu->GetTypeAt(i));
      EXPECT_EQ(mojo_item->command_id, menu->GetCommandIdAt(i));
      EXPECT_EQ(mojo_item->label, menu->GetLabelAt(i));
      if (mojo_item->type == ui::MenuModel::TYPE_SUBMENU) {
        VLOG(1) << "It's a submenu, let's do a recursion.";
        CheckMenuItemsMatched(mojo_item->submenu.value(),
                              menu->GetSubmenuModelAt(i));
        continue;
      }
      EXPECT_EQ(mojo_item->enabled, menu->IsEnabledAt(i));
      EXPECT_EQ(mojo_item->checked, menu->IsItemCheckedAt(i));
      EXPECT_EQ(mojo_item->radio_group_id, menu->GetGroupIdAt(i));
    }
  }

 private:
  std::unique_ptr<ui::SimpleMenuModel> menu_model_;
  MenuModelList submenu_models_;

  DISALLOW_COPY_AND_ASSIGN(MenuUtilsTest);
};

TEST_F(MenuUtilsTest, Basic) {
  ui::SimpleMenuModel* menu = root_menu();

  // Populates items into |menu| for testing.
  int command_id = 0;
  menu->AddItem(command_id++, ASCIIToUTF16("Item0"));
  menu->AddItem(command_id++, ASCIIToUTF16("Item1"));
  menu->AddSeparator(ui::NORMAL_SEPARATOR);
  menu->AddCheckItem(command_id++, ASCIIToUTF16("CheckItem0"));
  menu->AddCheckItem(command_id++, ASCIIToUTF16("CheckItem1"));
  menu->AddRadioItem(command_id++, ASCIIToUTF16("RadioItem0"),
                     0 /* group_id */);
  menu->AddRadioItem(command_id++, ASCIIToUTF16("RadioItem1"),
                     0 /* group_id */);
  // Creates a submenu.
  ui::SimpleMenuModel* submenu = CreateSubmenu();
  submenu->AddItem(command_id++, ASCIIToUTF16("SubMenu-Item0"));
  submenu->AddItem(command_id++, ASCIIToUTF16("SubMenu-Item1"));
  submenu->AddSeparator(ui::NORMAL_SEPARATOR);
  submenu->AddCheckItem(command_id++, ASCIIToUTF16("SubMenu-CheckItem0"));
  submenu->AddCheckItem(command_id++, ASCIIToUTF16("SubMenu-CheckItem1"));
  submenu->AddRadioItem(command_id++, ASCIIToUTF16("SubMenu-RadioItem0"),
                        1 /* group_id */);
  submenu->AddRadioItem(command_id++, ASCIIToUTF16("SubMenu-RadioItem1"),
                        1 /* group_id */);
  menu->AddSubMenu(command_id++, ASCIIToUTF16("SubMenu"), submenu);

  // Converts the menu into mojo format.
  MenuItemList mojo_menu_items = GetMojoMenuItemsFromModel(menu);
  CheckMenuItemsMatched(mojo_menu_items, menu);

  // Converts backwards.
  ui::SimpleMenuModel new_menu(this);
  SubmenuList new_submenus;
  PopulateMenuFromMojoMenuItems(&new_menu, this, mojo_menu_items,
                                &new_submenus);
  CheckMenuItemsMatched(mojo_menu_items, &new_menu);

  // Tests |GetMenuItemByCommandId|.
  for (int command_to_find = 0; command_to_find < command_id;
       command_to_find++) {
    // Gets the mojo item.
    const mojom::MenuItemPtr& mojo_item =
        GetMenuItemByCommandId(mojo_menu_items, command_to_find);

    // Gets the item index with this command from the original root menu.
    int index = menu->GetIndexOfCommandId(command_to_find);
    ui::MenuModel* menu_to_find = menu;
    if (index < 0) {
      // We cannot find it from the original root menu. Then it should be in the
      // submenu.
      index = submenu->GetIndexOfCommandId(command_to_find);
      EXPECT_LE(0, index);
      menu_to_find = submenu;
    }
    // Checks whether they match.
    EXPECT_EQ(mojo_item->type, menu_to_find->GetTypeAt(index));
    EXPECT_EQ(mojo_item->command_id, menu_to_find->GetCommandIdAt(index));
    EXPECT_EQ(mojo_item->label, menu_to_find->GetLabelAt(index));
    EXPECT_EQ(mojo_item->enabled, menu_to_find->IsEnabledAt(index));
    EXPECT_EQ(mojo_item->checked, menu_to_find->IsItemCheckedAt(index));
    EXPECT_EQ(mojo_item->radio_group_id, menu_to_find->GetGroupIdAt(index));
  }

  // For unknown command ids, we'll get a singleton stub item.
  const mojom::MenuItemPtr& item_not_found_1 =
      GetMenuItemByCommandId(mojo_menu_items, command_id + 1);
  const mojom::MenuItemPtr& item_not_found_2 =
      GetMenuItemByCommandId(mojo_menu_items, command_id + 2);
  EXPECT_EQ(item_not_found_1.get(), item_not_found_2.get());
}

}  // namespace menu_utils
}  // namespace ash
