// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/internal_app/internal_app_model_builder.h"

#include "base/macros.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/app_list/app_list_test_util.h"
#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "chrome/browser/ui/app_list/internal_app/internal_app_metadata.h"
#include "chrome/browser/ui/app_list/test/fake_app_list_model_updater.h"
#include "chrome/browser/ui/app_list/test/test_app_list_controller_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Get a set of all apps in |model|.
std::string GetModelContent(AppListModelUpdater* model_updater) {
  std::vector<std::string> app_names;
  for (size_t i = 0; i < model_updater->ItemCount(); ++i)
    app_names.emplace_back(model_updater->ItemAtForTest(i)->name());
  return base::JoinString(app_names, ",");
}

}  // namespace

class InternalAppModelBuilderTest : public AppListTestBase {
 public:
  InternalAppModelBuilderTest() {}
  ~InternalAppModelBuilderTest() override {}

  // AppListTestBase:
  void SetUp() override {
    AppListTestBase::SetUp();
    CreateBuilder();
  }

  void TearDown() override {
    ResetBuilder();
    AppListTestBase::TearDown();
  }

 protected:
  std::unique_ptr<FakeAppListModelUpdater> model_updater_;

 private:
  // Creates a new builder, destroying any existing one.
  void CreateBuilder() {
    ResetBuilder();  // Destroy any existing builder in the correct order.

    model_updater_ = std::make_unique<FakeAppListModelUpdater>();
    controller_ = std::make_unique<test::TestAppListControllerDelegate>();
    builder_ = std::make_unique<InternalAppModelBuilder>(controller_.get());
    builder_->Initialize(nullptr, profile(), model_updater_.get());
  }

  void ResetBuilder() {
    builder_.reset();
    controller_.reset();
    model_updater_.reset();
  }

  std::unique_ptr<test::TestAppListControllerDelegate> controller_;
  std::unique_ptr<InternalAppModelBuilder> builder_;

  DISALLOW_COPY_AND_ASSIGN(InternalAppModelBuilderTest);
};

TEST_F(InternalAppModelBuilderTest, Build) {
  // The internal apps list is provided by GetInternalAppList() in
  // internal_app_metadata.cc. Only count the apps can display in launcher.
  std::string internal_apps_name;
  EXPECT_EQ(app_list::GetNumberOfInternalAppsShowInLauncherForTest(
                &internal_apps_name),
            model_updater_->ItemCount());
  EXPECT_EQ(internal_apps_name, GetModelContent(model_updater_.get()));
}
