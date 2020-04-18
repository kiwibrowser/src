// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_file_system_id.h"

#include <string>

#include "base/files/file_path.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace smb_client {

class SmbFileSystemIdTest : public testing::Test {
 public:
  SmbFileSystemIdTest() = default;
  ~SmbFileSystemIdTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(SmbFileSystemIdTest);
};

TEST_F(SmbFileSystemIdTest, ShouldCreateFileSystemIdCorrectly) {
  const base::FilePath share_path("smb://192.168.0.0/test");
  const int32_t mount_id = 12;

  EXPECT_EQ("12@@smb://192.168.0.0/test",
            CreateFileSystemId(mount_id, share_path));
}

TEST_F(SmbFileSystemIdTest, ShouldParseMountIdCorrectly) {
  const std::string file_system_id = "12@@smb://192.168.0.0/test";

  EXPECT_EQ(12, GetMountIdFromFileSystemId(file_system_id));
}

TEST_F(SmbFileSystemIdTest, ShouldParseSharePathCorrectly) {
  const std::string file_system_id = "12@@smb://192.168.0.0/test";
  const base::FilePath expected_share_path("smb://192.168.0.0/test");

  EXPECT_EQ(expected_share_path, GetSharePathFromFileSystemId(file_system_id));
}

}  // namespace smb_client
}  // namespace chromeos
