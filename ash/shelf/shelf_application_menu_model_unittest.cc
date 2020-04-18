// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_application_menu_model.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {

namespace {

const char kNumItemsEnabledHistogramName[] =
    "Ash.Shelf.Menu.NumItemsEnabledUponSelection";

const char kSelectedMenuItemIndexHistogramName[] =
    "Ash.Shelf.Menu.SelectedMenuItemIndex";

}  // namespace

// Test API to provide internal access to a ShelfApplicationMenuModel.
class ShelfApplicationMenuModelTestAPI {
 public:
  // Creates a test api to access the internals of the |menu|.
  explicit ShelfApplicationMenuModelTestAPI(ShelfApplicationMenuModel* menu)
      : menu_(menu) {}
  ~ShelfApplicationMenuModelTestAPI() = default;

  // Give public access to this metrics recording functions.
  void RecordMenuItemSelectedMetrics(int command_id,
                                     int num_menu_items_enabled) {
    menu_->RecordMenuItemSelectedMetrics(command_id, num_menu_items_enabled);
  }

 private:
  // The ShelfApplicationMenuModel to provide internal access to. Not owned.
  ShelfApplicationMenuModel* menu_;

  DISALLOW_COPY_AND_ASSIGN(ShelfApplicationMenuModelTestAPI);
};

// Verifies the menu contents given an empty item list.
TEST(ShelfApplicationMenuModelTest, VerifyContentsWithNoMenuItems) {
  base::string16 title = base::ASCIIToUTF16("title");
  ShelfApplicationMenuModel menu(title, std::vector<mojom::MenuItemPtr>(),
                                 nullptr);
  // Expect the title with separators.
  ASSERT_EQ(static_cast<int>(3), menu.GetItemCount());
  EXPECT_EQ(ui::MenuModel::TYPE_SEPARATOR, menu.GetTypeAt(0));
  EXPECT_EQ(ui::MenuModel::TYPE_COMMAND, menu.GetTypeAt(1));
  EXPECT_EQ(title, menu.GetLabelAt(1));
  EXPECT_FALSE(menu.IsEnabledAt(1));
  EXPECT_EQ(ui::MenuModel::TYPE_SEPARATOR, menu.GetTypeAt(2));
}

// Verifies the menu contents given a non-empty item list.
TEST(ShelfApplicationMenuModelTest, VerifyContentsWithMenuItems) {
  std::vector<mojom::MenuItemPtr> items;
  base::string16 title1 = base::ASCIIToUTF16("title1");
  base::string16 title2 = base::ASCIIToUTF16("title2");
  base::string16 title3 = base::ASCIIToUTF16("title3");
  items.push_back(ash::mojom::MenuItem::New());
  items[0]->label = title1;
  items.push_back(ash::mojom::MenuItem::New());
  items[1]->label = title2;
  items.push_back(ash::mojom::MenuItem::New());
  items[2]->label = title3;

  base::string16 title = base::ASCIIToUTF16("title");
  ShelfApplicationMenuModel menu(title, std::move(items), nullptr);
  ShelfApplicationMenuModelTestAPI menu_test_api(&menu);

  // Expect the title with separators, the enabled items, and another separator.
  ASSERT_EQ(static_cast<int>(7), menu.GetItemCount());
  EXPECT_EQ(ui::MenuModel::TYPE_SEPARATOR, menu.GetTypeAt(0));
  EXPECT_EQ(ui::MenuModel::TYPE_COMMAND, menu.GetTypeAt(1));
  EXPECT_EQ(title, menu.GetLabelAt(1));
  EXPECT_FALSE(menu.IsEnabledAt(1));
  EXPECT_EQ(ui::MenuModel::TYPE_SEPARATOR, menu.GetTypeAt(2));
  EXPECT_EQ(ui::MenuModel::TYPE_COMMAND, menu.GetTypeAt(3));
  EXPECT_EQ(title1, menu.GetLabelAt(3));
  EXPECT_TRUE(menu.IsEnabledAt(3));
  EXPECT_EQ(ui::MenuModel::TYPE_COMMAND, menu.GetTypeAt(4));
  EXPECT_EQ(title2, menu.GetLabelAt(4));
  EXPECT_TRUE(menu.IsEnabledAt(4));
  EXPECT_EQ(ui::MenuModel::TYPE_COMMAND, menu.GetTypeAt(5));
  EXPECT_EQ(title3, menu.GetLabelAt(5));
  EXPECT_TRUE(menu.IsEnabledAt(5));
  EXPECT_EQ(ui::MenuModel::TYPE_SEPARATOR, menu.GetTypeAt(6));
}

// Verifies RecordMenuItemSelectedMetrics uses the correct histogram buckets.
TEST(ShelfApplicationMenuModelTest, VerifyHistogramBuckets) {
  const int kCommandId = 3;
  const int kNumMenuItemsEnabled = 7;

  base::HistogramTester histogram_tester;

  std::vector<mojom::MenuItemPtr> items;
  ShelfApplicationMenuModel menu(base::ASCIIToUTF16("title"), std::move(items),
                                 nullptr);
  ShelfApplicationMenuModelTestAPI menu_test_api(&menu);
  menu_test_api.RecordMenuItemSelectedMetrics(kCommandId, kNumMenuItemsEnabled);

  histogram_tester.ExpectTotalCount(kNumItemsEnabledHistogramName, 1);
  histogram_tester.ExpectBucketCount(kNumItemsEnabledHistogramName,
                                     kNumMenuItemsEnabled, 1);

  histogram_tester.ExpectTotalCount(kSelectedMenuItemIndexHistogramName, 1);
  histogram_tester.ExpectBucketCount(kSelectedMenuItemIndexHistogramName,
                                     kCommandId, 1);
}

// Verify histogram data is recorded when ExecuteCommand is called.
TEST(ShelfApplicationMenuModelTest, VerifyHistogramOnExecute) {
  base::HistogramTester histogram_tester;

  std::vector<mojom::MenuItemPtr> items;
  items.push_back(ash::mojom::MenuItem::New());
  base::string16 title = base::ASCIIToUTF16("title");
  ShelfApplicationMenuModel menu(title, std::move(items), nullptr);
  menu.ExecuteCommand(0, 0);

  histogram_tester.ExpectTotalCount(kNumItemsEnabledHistogramName, 1);
  histogram_tester.ExpectTotalCount(kSelectedMenuItemIndexHistogramName, 1);
}

}  // namespace ash
