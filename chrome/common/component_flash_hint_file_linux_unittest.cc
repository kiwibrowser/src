// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/component_flash_hint_file_linux.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mount.h>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/process/kill.h"
#include "base/test/multiprocess_test.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_timeouts.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace chrome {

class ComponentFlashHintFileTest : public base::MultiProcessTest {};

TEST_F(ComponentFlashHintFileTest, ExistsTest) {
  const base::ScopedPathOverride path_override(chrome::DIR_USER_DATA);
  EXPECT_FALSE(component_flash_hint_file::DoesHintFileExist());
}

TEST_F(ComponentFlashHintFileTest, InstallTest) {
  const base::ScopedPathOverride path_override(chrome::DIR_USER_DATA);
  EXPECT_FALSE(component_flash_hint_file::DoesHintFileExist());

  base::FilePath flash_dir;
  ASSERT_TRUE(base::PathService::Get(
      chrome::DIR_COMPONENT_UPDATED_PEPPER_FLASH_PLUGIN, &flash_dir));

  base::File::Error error;
  ASSERT_TRUE(base::CreateDirectoryAndGetError(flash_dir, &error));

  // Write out a fixed byte array as the flash file.
  uint8_t file[] = {0x4c, 0x65, 0x74, 0x20, 0x75, 0x73,
                    0x20, 0x6e, 0x6f, 0x74, 0x20, 0x67};
  flash_dir = flash_dir.Append("libflash.so");
  const std::string flash_version = "1.0.0.1";
  ASSERT_EQ(static_cast<int>(sizeof(file)),
            base::WriteFile(flash_dir, reinterpret_cast<const char*>(file),
                            sizeof(file)));
  ASSERT_TRUE(component_flash_hint_file::RecordFlashUpdate(flash_dir, flash_dir,
                                                           flash_version));
  ASSERT_TRUE(component_flash_hint_file::DoesHintFileExist());

  // Confirm that the flash plugin can be verified and returned.
  base::FilePath returned_flash_path;
  std::string version;
  ASSERT_TRUE(component_flash_hint_file::VerifyAndReturnFlashLocation(
      &returned_flash_path, &version));
  ASSERT_EQ(returned_flash_path, flash_dir);
  ASSERT_EQ(version, flash_version);

  // Now "corrupt" the flash file and make sure the checksum fails and nothing
  // is returned.
  file[0] = 0xAA;
  ASSERT_TRUE(base::WriteFile(flash_dir, reinterpret_cast<const char*>(file),
                              sizeof(file)) == sizeof(file));
  base::FilePath empty_path;
  std::string empty_version;
  ASSERT_FALSE(component_flash_hint_file::VerifyAndReturnFlashLocation(
      &empty_path, &empty_version));
  ASSERT_NE(empty_path, flash_dir);
  ASSERT_FALSE(empty_version == flash_version);
}

TEST_F(ComponentFlashHintFileTest, CorruptionTest) {
  const base::ScopedPathOverride path_override(chrome::DIR_USER_DATA);
  EXPECT_FALSE(component_flash_hint_file::DoesHintFileExist());

  base::FilePath flash_dir;
  ASSERT_TRUE(base::PathService::Get(
      chrome::DIR_COMPONENT_UPDATED_PEPPER_FLASH_PLUGIN, &flash_dir));

  base::File::Error error;
  ASSERT_TRUE(base::CreateDirectoryAndGetError(flash_dir, &error));
  flash_dir = flash_dir.Append("libflash.so");

  const uint8_t file[] = {0x56, 0x61, 0x20, 0x67, 0x75, 0x76,
                          0x66, 0x20, 0x62, 0x61, 0x72, 0x20};
  ASSERT_TRUE(base::WriteFile(flash_dir, reinterpret_cast<const char*>(file),
                              sizeof(file)) == sizeof(file));
  const std::string flash_version = "1.0.0.1";
  ASSERT_TRUE(component_flash_hint_file::RecordFlashUpdate(flash_dir, flash_dir,
                                                           flash_version));
  ASSERT_TRUE(component_flash_hint_file::DoesHintFileExist());

  // Now write out a new flash version that will not be moved into place.
  const uint8_t updated_file[] = {0x43, 0x72, 0x62, 0x63, 0x79, 0x72,
                                  0x20, 0x66, 0x7a, 0x76, 0x79, 0x76};
  base::FilePath flash_dir_update;
  ASSERT_TRUE(base::PathService::Get(
      chrome::DIR_COMPONENT_UPDATED_PEPPER_FLASH_PLUGIN, &flash_dir_update));
  flash_dir_update = flash_dir_update.Append("other_flash.so");
  ASSERT_TRUE(base::WriteFile(flash_dir_update,
                              reinterpret_cast<const char*>(updated_file),
                              sizeof(updated_file)) == sizeof(updated_file));
  ASSERT_TRUE(component_flash_hint_file::RecordFlashUpdate(
      flash_dir_update, flash_dir, flash_version));
  // |flash_dir_update| needs to be moved to |flash_dir|, but this test
  // deliberately skips that step, so VerifyAndReturnFlashLocation should fail.
  base::FilePath failed_flash_dir;
  std::string failed_version;
  ASSERT_FALSE(component_flash_hint_file::VerifyAndReturnFlashLocation(
      &failed_flash_dir, &failed_version));
}

TEST_F(ComponentFlashHintFileTest, ExecTest1) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath file_path = temp_dir.GetPath().Append("plugin.so");
  const uint8_t file[] = {0x55, 0x62, 0x79, 0x71, 0x20,
                          0x6c, 0x62, 0x68, 0x65, 0x20};

  ASSERT_TRUE(base::WriteFile(file_path, reinterpret_cast<const char*>(file),
                              sizeof(file)) == sizeof(file));
  ASSERT_TRUE(component_flash_hint_file::TestExecutableMapping(file_path));
}

MULTIPROCESS_TEST_MAIN(NoExecMountTest) {
  if (unshare(CLONE_NEWUSER | CLONE_NEWNS) != 0) {
    LOG(ERROR) << "This kernel does not support unprivileged namespaces. "
                  "ExecTest2 will succeed without running.";
    return 0;
  }
  // Now mount a NOEXEC fs.
  const unsigned long tmpfs_flags = MS_NODEV | MS_NOSUID | MS_NOEXEC;
  base::ScopedTempDir temp_dir;
  CHECK(temp_dir.CreateUniqueTempDir());
  CHECK_EQ(0, mount("tmpfs", temp_dir.GetPath().value().c_str(), "tmpfs",
                    tmpfs_flags, nullptr));
  const base::FilePath file_path = temp_dir.GetPath().Append("plugin.so");
  const uint8_t file[] = {0x56, 0x61, 0x20, 0x67, 0x75, 0x72,
                          0x20, 0x70, 0x76, 0x67, 0x6c, 0x20};
  bool test_exec = false;
  bool file_written =
      base::WriteFile(file_path, reinterpret_cast<const char*>(file),
                      sizeof(file)) == static_cast<int>(sizeof(file));
  if (file_written)
    test_exec = component_flash_hint_file::TestExecutableMapping(file_path);

  if (umount(temp_dir.GetPath().value().c_str()) != 0)
    LOG(ERROR) << "Could not unmount directory " << temp_dir.GetPath().value();

  CHECK(file_written);
  CHECK(!test_exec);
  return 0;
}

TEST_F(ComponentFlashHintFileTest, ExecTest2) {
  base::Process process = SpawnChild("NoExecMountTest");
  ASSERT_TRUE(process.IsValid());
  int exit_code = 42;
  ASSERT_TRUE(process.WaitForExitWithTimeout(TestTimeouts::action_max_timeout(),
                                             &exit_code));
  EXPECT_EQ(0, exit_code);
}

}  // namespace chrome
