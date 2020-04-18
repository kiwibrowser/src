// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/model/folder_image.h"

#include <string>
#include <utility>

#include "ash/app_list/model/app_list_item.h"
#include "ash/app_list/model/app_list_item_list.h"
#include "ash/app_list/model/app_list_model.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/skia_util.h"

namespace app_list {

namespace {

gfx::ImageSkia CreateSquareBitmapWithColor(int size, SkColor color) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(size, size);
  bitmap.eraseColor(color);
  return gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
}

bool ImagesAreEqual(const gfx::ImageSkia& image1,
                    const gfx::ImageSkia& image2) {
  return gfx::BitmapsAreEqual(*image1.bitmap(), *image2.bitmap());
}

// Listens for OnFolderImageUpdated and sets a flag upon receiving the signal.
class TestFolderImageObserver : public FolderImageObserver {
 public:
  TestFolderImageObserver() : updated_flag_(false) {}

  bool updated() const { return updated_flag_; }

  void Reset() { updated_flag_ = false; }

  // FolderImageObserver overrides:
  void OnFolderImageUpdated() override { updated_flag_ = true; }

 private:
  bool updated_flag_;

  DISALLOW_COPY_AND_ASSIGN(TestFolderImageObserver);
};

}  // namespace

class FolderImageTest : public testing::Test {
 public:
  FolderImageTest() : folder_image_(app_list_model_.top_level_item_list()) {}

  ~FolderImageTest() override {}

  void SetUp() override {
    // Populate the AppListModel with three items (to test that the FolderImage
    // correctly supports having fewer than four icons).
    AddAppWithColoredIcon("app1", SK_ColorRED);
    AddAppWithColoredIcon("app2", SK_ColorGREEN);
    AddAppWithColoredIcon("app3", SK_ColorBLUE);

    observer_.Reset();
    folder_image_.AddObserver(&observer_);
  }

  void TearDown() override { folder_image_.RemoveObserver(&observer_); }

 protected:
  void AddAppWithColoredIcon(const std::string& id, SkColor icon_color) {
    std::unique_ptr<AppListItem> item(new AppListItem(id));
    item->SetIcon(CreateSquareBitmapWithColor(kListIconSize, icon_color));
    app_list_model_.AddItem(std::move(item));
  }

  AppListModel app_list_model_;

  FolderImage folder_image_;

  TestFolderImageObserver observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FolderImageTest);
};

TEST_F(FolderImageTest, UpdateListTest) {
  gfx::ImageSkia icon1 = folder_image_.icon();

  // Call UpdateIcon and ensure that the observer event fired.
  folder_image_.UpdateIcon();
  EXPECT_TRUE(observer_.updated());
  observer_.Reset();
  // The icon should not have changed.
  EXPECT_TRUE(ImagesAreEqual(icon1, folder_image_.icon()));

  // Swap two items. Ensure that the observer fired and the icon changed.
  app_list_model_.top_level_item_list()->MoveItem(2, 1);
  EXPECT_TRUE(observer_.updated());
  observer_.Reset();
  gfx::ImageSkia icon2 = folder_image_.icon();
  EXPECT_FALSE(ImagesAreEqual(icon1, icon2));

  // Swap back items. Ensure that the observer fired and the icon changed back.
  app_list_model_.top_level_item_list()->MoveItem(2, 1);
  EXPECT_TRUE(observer_.updated());
  observer_.Reset();
  EXPECT_TRUE(ImagesAreEqual(icon1, folder_image_.icon()));

  // Add a new item. Ensure that the observer fired and the icon changed.
  AddAppWithColoredIcon("app4", SK_ColorYELLOW);
  EXPECT_TRUE(observer_.updated());
  observer_.Reset();
  gfx::ImageSkia icon3 = folder_image_.icon();
  EXPECT_FALSE(ImagesAreEqual(icon1, icon3));

  // Add a new item. The observer should not fire, nor should the icon change
  // (because it does not affect the first four icons).
  AddAppWithColoredIcon("app5", SK_ColorCYAN);
  EXPECT_FALSE(observer_.updated());
  observer_.Reset();
  EXPECT_TRUE(ImagesAreEqual(icon3, folder_image_.icon()));

  // Delete an item. Ensure that the observer fired and the icon changed.
  app_list_model_.DeleteItem("app2");
  EXPECT_TRUE(observer_.updated());
  observer_.Reset();
  gfx::ImageSkia icon4 = folder_image_.icon();
  EXPECT_FALSE(ImagesAreEqual(icon3, icon4));
}

TEST_F(FolderImageTest, UpdateItemTest) {
  gfx::ImageSkia icon1 = folder_image_.icon();

  // Change an item's icon. Ensure that the observer fired and the icon changed.
  app_list_model_.FindItem("app2")
      ->SetIcon(CreateSquareBitmapWithColor(kListIconSize, SK_ColorMAGENTA));
  EXPECT_TRUE(observer_.updated());
  observer_.Reset();
  EXPECT_FALSE(ImagesAreEqual(icon1, folder_image_.icon()));
}

}  // namespace app_list
