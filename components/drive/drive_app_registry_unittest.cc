// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/drive_app_registry.h"

#include <stddef.h>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "components/drive/drive_app_registry_observer.h"
#include "components/drive/service/fake_drive_service.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {

class TestDriveAppRegistryObserver : public DriveAppRegistryObserver {
 public:
  explicit TestDriveAppRegistryObserver(DriveAppRegistry* registry)
      : registry_(registry),
        update_count_(0) {
    registry_->AddObserver(this);
  }
  ~TestDriveAppRegistryObserver() override { registry_->RemoveObserver(this); }

  int update_count() const { return update_count_; }

 private:
  // DriveAppRegistryObserver overrides:
  void OnDriveAppRegistryUpdated() override { ++update_count_; }

  DriveAppRegistry* registry_;
  int update_count_;
  DISALLOW_COPY_AND_ASSIGN(TestDriveAppRegistryObserver);
};

class DriveAppRegistryTest : public testing::Test {
 protected:
  void SetUp() override {
    fake_drive_service_.reset(new FakeDriveService);
    fake_drive_service_->LoadAppListForDriveApi("drive/applist.json");

    apps_registry_.reset(new DriveAppRegistry(fake_drive_service_.get()));
  }

  bool VerifyApp(const std::vector<DriveAppInfo>& list,
                 const std::string& app_id,
                 const std::string& app_name) {
    bool found = false;
    for (size_t i = 0; i < list.size(); ++i) {
      const DriveAppInfo& app = list[i];
      if (app_id == app.app_id) {
        EXPECT_EQ(app_name, app.app_name);
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Unable to find app with app_id " << app_id;
    return found;
  }

  base::MessageLoop message_loop_;
  std::unique_ptr<FakeDriveService> fake_drive_service_;
  std::unique_ptr<DriveAppRegistry> apps_registry_;
};

TEST_F(DriveAppRegistryTest, BasicParse) {
  TestDriveAppRegistryObserver observer(apps_registry_.get());

  apps_registry_->Update();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, observer.update_count());

  std::vector<DriveAppInfo> apps;
  apps_registry_->GetAppList(&apps);

  ASSERT_EQ(2u, apps.size());
  EXPECT_EQ("123456788192", apps[0].app_id);
  EXPECT_EQ("Drive app 1", apps[0].app_name);
  EXPECT_EQ("https://www.example.com/createForApp1",
            apps[0].create_url.spec());
  EXPECT_EQ("abcdefghabcdefghabcdefghabcdefgh", apps[0].product_id);
  EXPECT_TRUE(apps[0].is_removable);
}

TEST_F(DriveAppRegistryTest, LoadAndFindDriveApps) {
  TestDriveAppRegistryObserver observer(apps_registry_.get());

  apps_registry_->Update();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, observer.update_count());

  // Find by primary extension 'exe'.
  std::vector<DriveAppInfo> ext_results;
  base::FilePath ext_file(FILE_PATH_LITERAL("drive/file.exe"));
  apps_registry_->GetAppsForFile(ext_file.Extension(), "", &ext_results);
  ASSERT_EQ(1U, ext_results.size());
  VerifyApp(ext_results, "123456788192", "Drive app 1");

  // Find by primary MIME type.
  std::vector<DriveAppInfo> primary_app;
  apps_registry_->GetAppsForFile(base::FilePath::StringType(),
      "application/vnd.google-apps.drive-sdk.123456788192", &primary_app);
  ASSERT_EQ(1U, primary_app.size());
  VerifyApp(primary_app, "123456788192", "Drive app 1");

  // Find by secondary MIME type.
  std::vector<DriveAppInfo> secondary_app;
  apps_registry_->GetAppsForFile(
      base::FilePath::StringType(), "text/html", &secondary_app);
  ASSERT_EQ(1U, secondary_app.size());
  VerifyApp(secondary_app, "123456788192", "Drive app 1");
}

TEST_F(DriveAppRegistryTest, UpdateFromAppList) {
  std::unique_ptr<base::Value> app_info_value =
      google_apis::test_util::LoadJSONFile("drive/applist.json");
  std::unique_ptr<google_apis::AppList> app_list(
      google_apis::AppList::CreateFrom(*app_info_value));

  TestDriveAppRegistryObserver observer(apps_registry_.get());
  apps_registry_->UpdateFromAppList(*app_list);
  EXPECT_EQ(1, observer.update_count());

  // Confirm that something was loaded from applist.json.
  std::vector<DriveAppInfo> ext_results;
  base::FilePath ext_file(FILE_PATH_LITERAL("drive/file.exe"));
  apps_registry_->GetAppsForFile(ext_file.Extension(), "", &ext_results);
  ASSERT_EQ(1U, ext_results.size());
}

TEST_F(DriveAppRegistryTest, MultipleUpdate) {
  TestDriveAppRegistryObserver observer(apps_registry_.get());

  // Call Update().
  apps_registry_->Update();
  EXPECT_EQ(0, observer.update_count());

  // Call Update() again.
  // This call should be ignored because there is already an ongoing update.
  apps_registry_->Update();
  EXPECT_EQ(0, observer.update_count());

  // The app list should be loaded only once.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, fake_drive_service_->app_list_load_count());
  EXPECT_EQ(1, observer.update_count());
}

TEST(DriveAppRegistryUtilTest, FindPreferredIcon_Empty) {
  DriveAppInfo::IconList icons;
  EXPECT_EQ("",
            util::FindPreferredIcon(icons, util::kPreferredIconSize).spec());
}

TEST(DriveAppRegistryUtilTest, FindPreferredIcon_) {
  const char kSmallerIconUrl[] = "http://example.com/smaller.png";
  const char kMediumIconUrl[] = "http://example.com/medium.png";
  const char kBiggerIconUrl[] = "http://example.com/bigger.png";
  const int kMediumSize = 16;

  DriveAppInfo::IconList icons;
  // The icons are not sorted by the size.
  icons.push_back(std::make_pair(kMediumSize,
                                 GURL(kMediumIconUrl)));
  icons.push_back(std::make_pair(kMediumSize + 2,
                                 GURL(kBiggerIconUrl)));
  icons.push_back(std::make_pair(kMediumSize - 2,
                                 GURL(kSmallerIconUrl)));

  // Exact match.
  EXPECT_EQ(kMediumIconUrl,
            util::FindPreferredIcon(icons, kMediumSize).spec());
  // The requested size is in-between of smaller.png and
  // medium.png. medium.png should be returned.
  EXPECT_EQ(kMediumIconUrl,
            util::FindPreferredIcon(icons, kMediumSize - 1).spec());
  // The requested size is smaller than the smallest icon. The smallest icon
  // should be returned.
  EXPECT_EQ(kSmallerIconUrl,
            util::FindPreferredIcon(icons, kMediumSize - 3).spec());
  // The requested size is larger than the largest icon. The largest icon
  // should be returned.
  EXPECT_EQ(kBiggerIconUrl,
            util::FindPreferredIcon(icons, kMediumSize + 3).spec());
}

TEST_F(DriveAppRegistryTest, UninstallDriveApp) {
  apps_registry_->Update();
  base::RunLoop().RunUntilIdle();

  std::vector<DriveAppInfo> apps;
  apps_registry_->GetAppList(&apps);
  size_t original_count = apps.size();

  // Uninstall an existing app.
  google_apis::DriveApiErrorCode error = google_apis::DRIVE_OTHER_ERROR;
  apps_registry_->UninstallApp(
      "123456788192",
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(error, google_apis::HTTP_NO_CONTENT);

  // Check that the number of apps is decreased by one.
  apps_registry_->GetAppList(&apps);
  EXPECT_EQ(original_count - 1, apps.size());

  // Try to uninstall a non-existing app.
  error = google_apis::DRIVE_OTHER_ERROR;
  apps_registry_->UninstallApp(
      "non-existing-app-id",
      google_apis::test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(error, google_apis::HTTP_NOT_FOUND);

  // Check that the number is not changed this time.
  apps_registry_->GetAppList(&apps);
  EXPECT_EQ(original_count - 1, apps.size());
}

}  // namespace drive
