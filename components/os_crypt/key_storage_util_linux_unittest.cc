// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/key_storage_util_linux.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using namespace os_crypt;

class KeyStorageUtilLinuxPreferenceTest : public testing::Test {
 public:
  KeyStorageUtilLinuxPreferenceTest() = default;
  ~KeyStorageUtilLinuxPreferenceTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(base::CreateNewTempDirectory("", &fake_user_data_dir_));
  }

  void TearDown() override {
    ASSERT_TRUE(base::DeleteFile(fake_user_data_dir_, true));
  }

 protected:
  base::FilePath fake_user_data_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyStorageUtilLinuxPreferenceTest);
};

TEST_F(KeyStorageUtilLinuxPreferenceTest, FirstTimeDefaultsToTrue) {
  EXPECT_TRUE(GetBackendUse(fake_user_data_dir_));
}

TEST_F(KeyStorageUtilLinuxPreferenceTest, SetToTrue) {
  EXPECT_TRUE(WriteBackendUse(fake_user_data_dir_, true));
  EXPECT_TRUE(GetBackendUse(fake_user_data_dir_));
}

TEST_F(KeyStorageUtilLinuxPreferenceTest, SetToFalse) {
  EXPECT_TRUE(WriteBackendUse(fake_user_data_dir_, false));
  EXPECT_FALSE(GetBackendUse(fake_user_data_dir_));
}

TEST_F(KeyStorageUtilLinuxPreferenceTest, MultipleWrites) {
  EXPECT_TRUE(WriteBackendUse(fake_user_data_dir_, false));
  EXPECT_FALSE(GetBackendUse(fake_user_data_dir_));

  EXPECT_TRUE(WriteBackendUse(fake_user_data_dir_, true));
  EXPECT_TRUE(GetBackendUse(fake_user_data_dir_));

  EXPECT_TRUE(WriteBackendUse(fake_user_data_dir_, false));
  EXPECT_FALSE(GetBackendUse(fake_user_data_dir_));
}

class KeyStorageUtilLinuxTest : public testing::Test {
 public:
  KeyStorageUtilLinuxTest() = default;
  ~KeyStorageUtilLinuxTest() override = default;

  void SetUp() override {}

  void TearDown() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyStorageUtilLinuxTest);
};

TEST_F(KeyStorageUtilLinuxTest, PasswordStoreFlagOverrides) {
  SelectedLinuxBackend selected;

  selected = SelectBackend(
      "basic", true, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_GNOME);
  EXPECT_EQ(selected, SelectedLinuxBackend::BASIC_TEXT);

  selected =
      SelectBackend("gnome-libsecret", false,
                    base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_KDE4);
  EXPECT_EQ(selected, SelectedLinuxBackend::GNOME_LIBSECRET);

  selected =
      SelectBackend("gnome-libsecret", true,
                    base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_KDE4);
  EXPECT_EQ(selected, SelectedLinuxBackend::GNOME_LIBSECRET);
}

TEST_F(KeyStorageUtilLinuxTest, IgnoreBackends) {
  SelectedLinuxBackend selected;

  selected = SelectBackend(
      "", true, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_GNOME);
  EXPECT_EQ(selected, SelectedLinuxBackend::GNOME_ANY);

  selected = SelectBackend(
      "", false, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_GNOME);
  EXPECT_EQ(selected, SelectedLinuxBackend::BASIC_TEXT);

  selected = SelectBackend(
      "", true, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_KDE5);
  EXPECT_EQ(selected, SelectedLinuxBackend::KWALLET5);

  selected = SelectBackend(
      "", false, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_KDE5);
  EXPECT_EQ(selected, SelectedLinuxBackend::BASIC_TEXT);
}

}  // namespace os_crypt
