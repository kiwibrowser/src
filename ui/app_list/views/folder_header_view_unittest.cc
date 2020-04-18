// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/folder_header_view.h"

#include <stddef.h>

#include <memory>
#include <string>

#include "ash/app_list/model/app_list_folder_item.h"
#include "ash/app_list/model/app_list_item.h"
#include "ash/app_list/model/app_list_model.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/app_list/test/app_list_test_model.h"
#include "ui/app_list/views/folder_header_view_delegate.h"
#include "ui/views/test/views_test_base.h"

namespace app_list {
namespace test {

namespace {

class TestFolderHeaderViewDelegate : public FolderHeaderViewDelegate {
 public:
  TestFolderHeaderViewDelegate() {}
  ~TestFolderHeaderViewDelegate() override {}

  // FolderHeaderViewDelegate
  void NavigateBack(AppListFolderItem* item,
                    const ui::Event& event_flags) override {}

  void GiveBackFocusToSearchBox() override {}

  void SetItemName(AppListFolderItem* item, const std::string& name) override {
    folder_name_ = name;
  }

  const std::string& folder_name() const { return folder_name_; }

 private:
  std::string folder_name_;

  DISALLOW_COPY_AND_ASSIGN(TestFolderHeaderViewDelegate);
};

}  // namespace

class FolderHeaderViewTest : public views::ViewsTestBase {
 public:
  FolderHeaderViewTest() {}
  ~FolderHeaderViewTest() override {}

  // testing::Test overrides:
  void SetUp() override {
    views::ViewsTestBase::SetUp();
    model_ = std::make_unique<AppListTestModel>();
    delegate_ = std::make_unique<TestFolderHeaderViewDelegate>();

    // Create a widget so that the FolderNameView can be focused.
    widget_ = std::make_unique<views::Widget>();
    views::Widget::InitParams params = views::ViewsTestBase::CreateParams(
        views::Widget::InitParams::TYPE_POPUP);
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    params.bounds = gfx::Rect(0, 0, 650, 650);
    widget_->Init(params);
    widget_->Show();

    folder_header_view_ = std::make_unique<FolderHeaderView>(delegate_.get());
    widget_->SetContentsView(folder_header_view_.get());
  }

  void TearDown() override {
    widget_->Close();
    folder_header_view_.reset();  // Release apps grid view before models.
    delegate_.reset();
    views::ViewsTestBase::TearDown();
  }

 protected:
  void UpdateFolderName(const std::string& name) {
    base::string16 folder_name = base::UTF8ToUTF16(name);
    folder_header_view_->SetFolderNameForTest(folder_name);
    folder_header_view_->ContentsChanged(NULL, folder_name);
  }

  const std::string GetFolderNameFromUI() {
    return base::UTF16ToUTF8(folder_header_view_->GetFolderNameForTest());
  }

  bool CanEditFolderName() {
    return folder_header_view_->IsFolderNameEnabledForTest();
  }

  std::unique_ptr<AppListTestModel> model_;
  std::unique_ptr<FolderHeaderView> folder_header_view_;
  std::unique_ptr<TestFolderHeaderViewDelegate> delegate_;
  std::unique_ptr<views::Widget> widget_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FolderHeaderViewTest);
};

TEST_F(FolderHeaderViewTest, SetFolderName) {
  // Creating a folder with empty folder name.
  AppListFolderItem* folder_item = model_->CreateAndPopulateFolderWithApps(2);
  folder_header_view_->SetFolderItem(folder_item);
  EXPECT_EQ("", GetFolderNameFromUI());
  EXPECT_TRUE(CanEditFolderName());

  // Update UI to set folder name to "test folder".
  UpdateFolderName("test folder");
  EXPECT_EQ("test folder", delegate_->folder_name());
}

TEST_F(FolderHeaderViewTest, WhitespaceCollapsedWhenFolderNameViewLosesFocus) {
  AppListFolderItem* folder_item = model_->CreateAndPopulateFolderWithApps(2);
  folder_header_view_->SetFolderItem(folder_item);
  views::View* name_view = folder_header_view_->GetFolderNameViewForTest();

  name_view->RequestFocus();
  UpdateFolderName("  N     A  ");
  widget_->GetFocusManager()->ClearFocus();

  // Expect that the folder name contains the same string with collapsed
  // whitespace.
  EXPECT_EQ("N A", delegate_->folder_name());
}

TEST_F(FolderHeaderViewTest, MaxFoldernNameLength) {
  // Creating a folder with empty folder name.
  AppListFolderItem* folder_item = model_->CreateAndPopulateFolderWithApps(2);
  folder_header_view_->SetFolderItem(folder_item);
  EXPECT_EQ("", GetFolderNameFromUI());
  EXPECT_TRUE(CanEditFolderName());

  // Update UI to set folder name to really long one beyond its maxium limit,
  // The folder name should be trucated to the maximum length.
  std::string max_len_name;
  for (size_t i = 0; i < kMaxFolderNameChars; ++i)
    max_len_name += "a";
  UpdateFolderName(max_len_name);
  EXPECT_EQ(max_len_name, delegate_->folder_name());
  std::string too_long_name = max_len_name + "a";
  UpdateFolderName(too_long_name);
  EXPECT_EQ(max_len_name, delegate_->folder_name());
}

TEST_F(FolderHeaderViewTest, OemFolderNameNotEditable) {
  AppListFolderItem* folder_item = model_->CreateAndAddOemFolder();
  folder_header_view_->SetFolderItem(folder_item);
  EXPECT_EQ("", GetFolderNameFromUI());
  EXPECT_FALSE(CanEditFolderName());
}

}  // namespace test
}  // namespace app_list
