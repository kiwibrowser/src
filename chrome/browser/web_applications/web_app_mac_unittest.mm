// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/web_applications/web_app_mac.h"

#import <Cocoa/Cocoa.h>
#include <errno.h>
#include <stddef.h>
#include <sys/xattr.h>

#include <memory>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#import "chrome/common/mac/app_mode_common.h"
#include "chrome/grit/theme_resources.h"
#include "components/version_info/version_info.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

namespace {

const char kFakeChromeBundleId[] = "fake.cfbundleidentifier";

class WebAppShortcutCreatorMock : public web_app::WebAppShortcutCreator {
 public:
  WebAppShortcutCreatorMock(const base::FilePath& app_data_dir,
                            const web_app::ShortcutInfo* shortcut_info)
      : WebAppShortcutCreator(app_data_dir, shortcut_info) {}

  MOCK_CONST_METHOD0(GetApplicationsDirname, base::FilePath());
  MOCK_CONST_METHOD1(GetAppBundleById,
                     base::FilePath(const std::string& bundle_id));
  MOCK_CONST_METHOD0(RevealAppShimInFinder, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(WebAppShortcutCreatorMock);
};

std::unique_ptr<web_app::ShortcutInfo> GetShortcutInfo() {
  std::unique_ptr<web_app::ShortcutInfo> info(new web_app::ShortcutInfo);
  info->extension_id = "extensionid";
  info->extension_path = base::FilePath("/fake/extension/path");
  info->title = base::ASCIIToUTF16("Shortcut Title");
  info->url = GURL("http://example.com/");
  info->profile_path = base::FilePath("user_data_dir").Append("Profile 1");
  info->profile_name = "profile name";
  info->version_for_display = "stable 1.0";
  info->from_bookmark = false;
  return info;
}

class WebAppShortcutCreatorTest : public testing::Test {
 protected:
  WebAppShortcutCreatorTest() {}

  void SetUp() override {
    base::mac::SetBaseBundleID(kFakeChromeBundleId);

    EXPECT_TRUE(temp_app_data_dir_.CreateUniqueTempDir());
    EXPECT_TRUE(temp_destination_dir_.CreateUniqueTempDir());
    app_data_dir_ = temp_app_data_dir_.GetPath();
    destination_dir_ = temp_destination_dir_.GetPath();

    info_ = GetShortcutInfo();
    shim_base_name_ = base::FilePath(info_->profile_path.BaseName().value() +
                                     " " + info_->extension_id + ".app");
    internal_shim_path_ = app_data_dir_.Append(shim_base_name_);
    shim_path_ = destination_dir_.Append(shim_base_name_);
  }

  // Needed by DCHECK_CURRENTLY_ON in ShortcutInfo destructor.
  content::TestBrowserThreadBundle thread_bundle_;

  base::ScopedTempDir temp_app_data_dir_;
  base::ScopedTempDir temp_destination_dir_;
  base::FilePath app_data_dir_;
  base::FilePath destination_dir_;

  std::unique_ptr<web_app::ShortcutInfo> info_;
  base::FilePath shim_base_name_;
  base::FilePath internal_shim_path_;
  base::FilePath shim_path_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebAppShortcutCreatorTest);
};


}  // namespace

namespace web_app {

TEST_F(WebAppShortcutCreatorTest, CreateShortcuts) {
  NiceMock<WebAppShortcutCreatorMock> shortcut_creator(app_data_dir_,
                                                       info_.get());
  EXPECT_CALL(shortcut_creator, GetApplicationsDirname())
      .WillRepeatedly(Return(destination_dir_));
  base::FilePath strings_file =
      destination_dir_.Append(".localized").Append("en_US.strings");

  // The Chrome Apps folder shouldn't be localized yet.
  EXPECT_FALSE(base::PathExists(strings_file));

  EXPECT_TRUE(shortcut_creator.CreateShortcuts(
      SHORTCUT_CREATION_AUTOMATED, web_app::ShortcutLocations()));
  EXPECT_TRUE(base::PathExists(shim_path_));
  EXPECT_TRUE(base::PathExists(destination_dir_));
  EXPECT_EQ(shim_base_name_, shortcut_creator.GetShortcutBasename());

  // When a shortcut is created, the parent, "Chrome Apps" folder should become
  // localized, but only once, to avoid concurrency issues in NSWorkspace. Note
  // this will fail if the CreateShortcuts test is run multiple times in the
  // same process, but the test runner should never do that.
  EXPECT_TRUE(base::PathExists(strings_file));

  // Delete it here, just to test that it is not recreated.
  EXPECT_TRUE(base::DeleteFile(strings_file, true));

  // Ensure the strings file wasn't recreated. It's not needed for any other
  // tests.
  EXPECT_TRUE(shortcut_creator.CreateShortcuts(SHORTCUT_CREATION_AUTOMATED,
                                               web_app::ShortcutLocations()));
  EXPECT_FALSE(base::PathExists(strings_file));

  base::FilePath plist_path =
      shim_path_.Append("Contents").Append("Info.plist");
  NSDictionary* plist = [NSDictionary dictionaryWithContentsOfFile:
      base::mac::FilePathToNSString(plist_path)];
  EXPECT_NSEQ(base::SysUTF8ToNSString(info_->extension_id),
              [plist objectForKey:app_mode::kCrAppModeShortcutIDKey]);
  EXPECT_NSEQ(base::SysUTF16ToNSString(info_->title),
              [plist objectForKey:app_mode::kCrAppModeShortcutNameKey]);
  EXPECT_NSEQ(base::SysUTF8ToNSString(info_->url.spec()),
              [plist objectForKey:app_mode::kCrAppModeShortcutURLKey]);

  EXPECT_NSEQ(base::SysUTF8ToNSString(version_info::GetVersionNumber()),
              [plist objectForKey:app_mode::kCrBundleVersionKey]);
  EXPECT_NSEQ(base::SysUTF8ToNSString(info_->version_for_display),
              [plist objectForKey:app_mode::kCFBundleShortVersionStringKey]);

  // Make sure all values in the plist are actually filled in.
  for (id key in plist) {
    id value = [plist valueForKey:key];
    if (!base::mac::ObjCCast<NSString>(value))
      continue;

    EXPECT_EQ(static_cast<NSUInteger>(NSNotFound),
              [value rangeOfString:@"@APP_"].location)
        << [key UTF8String] << ":" << [value UTF8String];
  }
}

TEST_F(WebAppShortcutCreatorTest, UpdateShortcuts) {
  base::ScopedTempDir other_folder_temp_dir;
  EXPECT_TRUE(other_folder_temp_dir.CreateUniqueTempDir());
  base::FilePath other_folder = other_folder_temp_dir.GetPath();
  base::FilePath other_shim_path = other_folder.Append(shim_base_name_);

  NiceMock<WebAppShortcutCreatorMock> shortcut_creator(app_data_dir_,
                                                       info_.get());
  EXPECT_CALL(shortcut_creator, GetApplicationsDirname())
      .WillRepeatedly(Return(destination_dir_));

  std::string expected_bundle_id = kFakeChromeBundleId;
  expected_bundle_id += ".app.Profile-1-" + info_->extension_id;
  EXPECT_CALL(shortcut_creator, GetAppBundleById(expected_bundle_id))
      .WillOnce(Return(other_shim_path));

  EXPECT_TRUE(shortcut_creator.BuildShortcut(other_shim_path));

  EXPECT_TRUE(base::DeleteFile(other_shim_path.Append("Contents"), true));

  EXPECT_TRUE(shortcut_creator.UpdateShortcuts());
  EXPECT_FALSE(base::PathExists(shim_path_));
  EXPECT_TRUE(base::PathExists(other_shim_path.Append("Contents")));

  // Also test case where GetAppBundleById fails.
  EXPECT_CALL(shortcut_creator, GetAppBundleById(expected_bundle_id))
      .WillOnce(Return(base::FilePath()));

  EXPECT_TRUE(shortcut_creator.BuildShortcut(other_shim_path));

  EXPECT_TRUE(base::DeleteFile(other_shim_path.Append("Contents"), true));

  EXPECT_FALSE(shortcut_creator.UpdateShortcuts());
  EXPECT_FALSE(base::PathExists(shim_path_));
  EXPECT_FALSE(base::PathExists(other_shim_path.Append("Contents")));
}

TEST_F(WebAppShortcutCreatorTest, UpdateBookmarkAppShortcut) {
  base::ScopedTempDir other_folder_temp_dir;
  EXPECT_TRUE(other_folder_temp_dir.CreateUniqueTempDir());
  base::FilePath other_folder = other_folder_temp_dir.GetPath();
  base::FilePath other_shim_path = other_folder.Append(shim_base_name_);
  info_->from_bookmark = true;

  NiceMock<WebAppShortcutCreatorMock> shortcut_creator(app_data_dir_,
                                                       info_.get());
  EXPECT_CALL(shortcut_creator, GetApplicationsDirname())
      .WillRepeatedly(Return(destination_dir_));

  std::string expected_bundle_id = kFakeChromeBundleId;
  expected_bundle_id += ".app.Profile-1-" + info_->extension_id;

  EXPECT_CALL(shortcut_creator, GetAppBundleById(expected_bundle_id))
      .WillOnce(Return(shim_path_));

  EXPECT_TRUE(shortcut_creator.BuildShortcut(other_shim_path));

  EXPECT_TRUE(base::DeleteFile(other_shim_path, true));

  // The original shim should be recreated.
  EXPECT_TRUE(shortcut_creator.UpdateShortcuts());
  EXPECT_TRUE(base::PathExists(shim_path_));
  EXPECT_FALSE(base::PathExists(other_shim_path.Append("Contents")));
}

TEST_F(WebAppShortcutCreatorTest, DeleteShortcuts) {
  // When using base::PathService::Override, it calls
  // base::MakeAbsoluteFilePath. On Mac this prepends "/private" to the path,
  // but points to the same directory in the file system.
  app_data_dir_ = base::MakeAbsoluteFilePath(app_data_dir_);

  base::ScopedTempDir other_folder_temp_dir;
  EXPECT_TRUE(other_folder_temp_dir.CreateUniqueTempDir());
  base::FilePath other_folder = other_folder_temp_dir.GetPath();
  base::FilePath other_shim_path = other_folder.Append(shim_base_name_);

  NiceMock<WebAppShortcutCreatorMock> shortcut_creator(app_data_dir_,
                                                       info_.get());
  EXPECT_CALL(shortcut_creator, GetApplicationsDirname())
      .WillRepeatedly(Return(destination_dir_));

  std::string expected_bundle_id = kFakeChromeBundleId;
  expected_bundle_id += ".app.Profile-1-" + info_->extension_id;
  EXPECT_CALL(shortcut_creator, GetAppBundleById(expected_bundle_id))
      .WillOnce(Return(other_shim_path));

  EXPECT_TRUE(shortcut_creator.CreateShortcuts(
      SHORTCUT_CREATION_AUTOMATED, web_app::ShortcutLocations()));
  EXPECT_TRUE(base::PathExists(internal_shim_path_));
  EXPECT_TRUE(base::PathExists(shim_path_));

  // Create an extra shim in another folder. It should be deleted since its
  // bundle id matches.
  EXPECT_TRUE(shortcut_creator.BuildShortcut(other_shim_path));
  EXPECT_TRUE(base::PathExists(other_shim_path));

  // Change the user_data_dir of the shim at shim_path_. It should not be
  // deleted since its user_data_dir does not match.
  NSString* plist_path = base::mac::FilePathToNSString(
      shim_path_.Append("Contents").Append("Info.plist"));
  NSMutableDictionary* plist =
      [NSMutableDictionary dictionaryWithContentsOfFile:plist_path];
  [plist setObject:@"fake_user_data_dir"
            forKey:app_mode::kCrAppModeUserDataDirKey];
  [plist writeToFile:plist_path
          atomically:YES];

  EXPECT_TRUE(
      base::PathService::Override(chrome::DIR_USER_DATA, app_data_dir_));
  shortcut_creator.DeleteShortcuts();
  EXPECT_FALSE(base::PathExists(internal_shim_path_));
  EXPECT_TRUE(base::PathExists(shim_path_));
  EXPECT_FALSE(base::PathExists(other_shim_path));
}

TEST_F(WebAppShortcutCreatorTest, CreateAppListShortcut) {
  // With an empty |profile_name|, the shortcut path should not have the profile
  // directory prepended to the extension id on the app bundle name.
  info_->profile_name.clear();
  base::FilePath dst_path =
      destination_dir_.Append(info_->extension_id + ".app");

  NiceMock<WebAppShortcutCreatorMock> shortcut_creator(base::FilePath(),
                                                       info_.get());
  EXPECT_CALL(shortcut_creator, GetApplicationsDirname())
      .WillRepeatedly(Return(destination_dir_));
  EXPECT_EQ(dst_path.BaseName(), shortcut_creator.GetShortcutBasename());
}

TEST_F(WebAppShortcutCreatorTest, RunShortcut) {
  NiceMock<WebAppShortcutCreatorMock> shortcut_creator(app_data_dir_,
                                                       info_.get());
  EXPECT_CALL(shortcut_creator, GetApplicationsDirname())
      .WillRepeatedly(Return(destination_dir_));

  EXPECT_TRUE(shortcut_creator.CreateShortcuts(
      SHORTCUT_CREATION_AUTOMATED, web_app::ShortcutLocations()));
  EXPECT_TRUE(base::PathExists(shim_path_));

  ssize_t status = getxattr(
      shim_path_.value().c_str(), "com.apple.quarantine", NULL, 0, 0, 0);
  EXPECT_EQ(-1, status);
  EXPECT_EQ(ENOATTR, errno);
}

TEST_F(WebAppShortcutCreatorTest, CreateFailure) {
  base::FilePath non_existent_path =
      destination_dir_.Append("not-existent").Append("name.app");

  NiceMock<WebAppShortcutCreatorMock> shortcut_creator(app_data_dir_,
                                                       info_.get());
  EXPECT_CALL(shortcut_creator, GetApplicationsDirname())
      .WillRepeatedly(Return(non_existent_path));
  EXPECT_FALSE(shortcut_creator.CreateShortcuts(
      SHORTCUT_CREATION_AUTOMATED, web_app::ShortcutLocations()));
}

TEST_F(WebAppShortcutCreatorTest, UpdateIcon) {
  gfx::Image product_logo =
      ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
          IDR_PRODUCT_LOGO_32);
  info_->favicon.Add(product_logo);
  WebAppShortcutCreatorMock shortcut_creator(app_data_dir_, info_.get());

  ASSERT_TRUE(shortcut_creator.UpdateIcon(shim_path_));
  base::FilePath icon_path =
      shim_path_.Append("Contents").Append("Resources").Append("app.icns");

  base::scoped_nsobject<NSImage> image([[NSImage alloc]
      initWithContentsOfFile:base::mac::FilePathToNSString(icon_path)]);
  EXPECT_TRUE(image);
  EXPECT_EQ(product_logo.Width(), [image size].width);
  EXPECT_EQ(product_logo.Height(), [image size].height);
}

// Disabled, sometimes crashes on "Mac10.10 tests". https://crbug.com/741642
TEST_F(WebAppShortcutCreatorTest, DISABLED_RevealAppShimInFinder) {
  WebAppShortcutCreatorMock shortcut_creator(app_data_dir_, info_.get());
  EXPECT_CALL(shortcut_creator, GetApplicationsDirname())
      .WillRepeatedly(Return(destination_dir_));

  EXPECT_CALL(shortcut_creator, RevealAppShimInFinder())
      .Times(0);
  EXPECT_TRUE(shortcut_creator.CreateShortcuts(
      SHORTCUT_CREATION_AUTOMATED, web_app::ShortcutLocations()));

  EXPECT_CALL(shortcut_creator, RevealAppShimInFinder());
  EXPECT_TRUE(shortcut_creator.CreateShortcuts(
      SHORTCUT_CREATION_BY_USER, web_app::ShortcutLocations()));
}

}  // namespace web_app
