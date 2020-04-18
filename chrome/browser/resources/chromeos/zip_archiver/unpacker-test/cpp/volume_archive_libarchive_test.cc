// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "volume_archive_libarchive.h"

#include "fake_lib_archive.h"
#include "fake_volume_reader.h"
#include "testing/gtest/gtest.h"

namespace {

// Fake default character encoding for the archive headers.
const char kEncoding[] = "CP1250";

}  // namespace

// Class used by TEST_F macro to initialize the environment for testing
// VolumeArchiveLibarchive methods except for Read.
class VolumeArchiveLibarchiveTest : public testing::Test {
 protected:
  VolumeArchiveLibarchiveTest() : volume_archive(NULL) {}

  virtual void SetUp() {
    fake_lib_archive_config::ResetVariables();
    // Pass FakeVolumeReader ownership to VolumeArchiveLibarchive.
    volume_archive = new VolumeArchiveLibarchive(new FakeVolumeReader());
  }

  virtual void TearDown() {
    volume_archive->Cleanup();
    delete volume_archive;
    volume_archive = NULL;
  }

  VolumeArchiveLibarchive* volume_archive;
};

TEST_F(VolumeArchiveLibarchiveTest, InitArchiveNewFailure) {
  // Test archive_read_new failure.
  fake_lib_archive_config::fail_archive_read_new = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));
  EXPECT_EQ(volume_archive_constants::kArchiveReadNewError,
            volume_archive->error_message());
}

TEST_F(VolumeArchiveLibarchiveTest, InitArchiveSupportFailures) {
  // Test rar support failure.
  fake_lib_archive_config::fail_archive_rar_support = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));

  std::string support_error =
      std::string(volume_archive_constants::kArchiveSupportErrorPrefix) +
      fake_lib_archive_config::kArchiveError;
  EXPECT_EQ(support_error, volume_archive->error_message());

  // Test zip support failure.
  fake_lib_archive_config::fail_archive_rar_support = false;
  fake_lib_archive_config::fail_archive_zip_seekable_support = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));
  EXPECT_EQ(support_error, volume_archive->error_message());
}

TEST_F(VolumeArchiveLibarchiveTest, InitOpenFailures) {
  // Test set read callback failure.
  fake_lib_archive_config::fail_archive_set_read_callback = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));

  std::string open_error =
      std::string(volume_archive_constants::kArchiveOpenErrorPrefix) +
      fake_lib_archive_config::kArchiveError;
  EXPECT_EQ(open_error, volume_archive->error_message());

  // Test set skip callback failure.
  fake_lib_archive_config::fail_archive_set_read_callback = false;
  fake_lib_archive_config::fail_archive_set_skip_callback = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));
  EXPECT_EQ(open_error, volume_archive->error_message());

  // Test set seek callback failure.
  fake_lib_archive_config::fail_archive_set_skip_callback = false;
  fake_lib_archive_config::fail_archive_set_seek_callback = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));
  EXPECT_EQ(open_error, volume_archive->error_message());

  // Test set close callback failure.
  fake_lib_archive_config::fail_archive_set_seek_callback = false;
  fake_lib_archive_config::fail_archive_set_close_callback = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));
  EXPECT_EQ(open_error, volume_archive->error_message());

  // Test set callback data failure.
  fake_lib_archive_config::fail_archive_set_close_callback = false;
  fake_lib_archive_config::fail_archive_set_callback_data = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));
  EXPECT_EQ(open_error, volume_archive->error_message());

  // Test archive open failure.
  fake_lib_archive_config::fail_archive_set_callback_data = false;
  fake_lib_archive_config::fail_archive_read_open = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));
  EXPECT_EQ(open_error, volume_archive->error_message());

  // Test set options callback failure.
  std::string support_error =
      std::string(volume_archive_constants::kArchiveSupportErrorPrefix) +
      fake_lib_archive_config::kArchiveError;

  fake_lib_archive_config::fail_archive_read_open = false;
  fake_lib_archive_config::fail_archive_set_options = true;
  EXPECT_FALSE(volume_archive->Init(kEncoding));
  EXPECT_EQ(support_error, volume_archive->error_message());
}

TEST_F(VolumeArchiveLibarchiveTest, InitSuccess) {
  EXPECT_TRUE(volume_archive->Init(kEncoding));
}

TEST_F(VolumeArchiveLibarchiveTest, InitWithEmptyEncoding) {
  // Make sure that archive_set_options is not called by making it return an
  // error.
  fake_lib_archive_config::fail_archive_set_options = true;
  EXPECT_TRUE(volume_archive->Init(""));
}

TEST_F(VolumeArchiveLibarchiveTest, GetNextHeaderSuccess) {
  std::string expected_path_name =
      std::string(fake_lib_archive_config::kPathName);
  const char* path_name = NULL;
  int64_t size = 0;
  bool is_directory = false;
  time_t modification_time = 0;

  // Test GetNextHeader for files.
  fake_lib_archive_config::archive_read_next_header_return_value = ARCHIVE_OK;
  fake_lib_archive_config::archive_entry_filetype_return_value =
      S_IFREG;  // Regular file.

  EXPECT_EQ(VolumeArchive::RESULT_SUCCESS,
            volume_archive->GetNextHeader(&path_name, &size, &is_directory,
                                          &modification_time));
  EXPECT_EQ(expected_path_name, path_name);
  EXPECT_EQ(fake_lib_archive_config::kSize, size);
  EXPECT_EQ(fake_lib_archive_config::kModificationTime, modification_time);
  EXPECT_FALSE(is_directory);

  // Test GetNextHeader for directories.
  fake_lib_archive_config::archive_entry_filetype_return_value =
      S_IFDIR;  // Directory.

  EXPECT_EQ(VolumeArchive::RESULT_SUCCESS,
            volume_archive->GetNextHeader(&path_name, &size, &is_directory,
                                          &modification_time));
  EXPECT_EQ(expected_path_name, path_name);
  EXPECT_EQ(fake_lib_archive_config::kSize, size);
  EXPECT_EQ(fake_lib_archive_config::kModificationTime, modification_time);
  EXPECT_TRUE(is_directory);
}

TEST_F(VolumeArchiveLibarchiveTest, GetNextHeaderEndOfArchive) {
  EXPECT_TRUE(volume_archive->Init(kEncoding));

  // Test GetNextHeader when at the end of archive.
  fake_lib_archive_config::archive_read_next_header_return_value = ARCHIVE_EOF;
  EXPECT_EQ(VolumeArchive::RESULT_EOF, volume_archive->GetNextHeader());
}

TEST_F(VolumeArchiveLibarchiveTest, GetNextHeaderFailure) {
  EXPECT_TRUE(volume_archive->Init(kEncoding));

  // Test failure GetNextHeader.
  fake_lib_archive_config::archive_read_next_header_return_value =
      ARCHIVE_FATAL;
  EXPECT_EQ(VolumeArchive::RESULT_FAIL, volume_archive->GetNextHeader());

  std::string next_header_error =
      std::string(volume_archive_constants::kArchiveNextHeaderErrorPrefix) +
      fake_lib_archive_config::kArchiveError;
  EXPECT_EQ(next_header_error, volume_archive->error_message());
}

TEST_F(VolumeArchiveLibarchiveTest, GetNextHeaderFailureArgs) {
  EXPECT_TRUE(volume_archive->Init(kEncoding));

  // Test failure GetNextHeader.
  fake_lib_archive_config::archive_read_next_header_return_value =
      ARCHIVE_FATAL;
  const char* pathname = NULL;
  int64_t size = 0;
  bool is_directory = false;
  time_t modification_time = 0;

  EXPECT_EQ(VolumeArchive::RESULT_FAIL,
            volume_archive->GetNextHeader(&pathname, &size, &is_directory,
                                          &modification_time));

  std::string next_header_error =
      std::string(volume_archive_constants::kArchiveNextHeaderErrorPrefix) +
      fake_lib_archive_config::kArchiveError;
  EXPECT_EQ(next_header_error, volume_archive->error_message());
}

TEST_F(VolumeArchiveLibarchiveTest, CleanupSuccess) {
  EXPECT_TRUE(volume_archive->reader() != NULL);
  EXPECT_TRUE(volume_archive->Init(kEncoding));

  // Test successful Cleanup after successful Init.
  EXPECT_TRUE(volume_archive->Cleanup());
  EXPECT_EQ(NULL, volume_archive->reader());
}

TEST_F(VolumeArchiveLibarchiveTest, CleanupFailure) {
  EXPECT_TRUE(volume_archive->reader() != NULL);
  EXPECT_TRUE(volume_archive->Init(kEncoding));

  // Test failure Cleanup after successful Init.
  fake_lib_archive_config::fail_archive_read_free = true;
  EXPECT_TRUE(!volume_archive->Cleanup());

  std::string free_error =
      std::string(volume_archive_constants::kArchiveReadFreeErrorPrefix) +
      fake_lib_archive_config::kArchiveError;
  EXPECT_EQ(free_error, volume_archive->error_message());
  EXPECT_EQ(NULL, volume_archive->reader());
}

TEST_F(VolumeArchiveLibarchiveTest, CleanupAfterCleanup) {
  EXPECT_TRUE(volume_archive->reader() != NULL);
  EXPECT_TRUE(volume_archive->Init(kEncoding));

  // Test Cleanup after Cleanup.
  EXPECT_TRUE(volume_archive->Cleanup());
  EXPECT_EQ(NULL, volume_archive->reader());

  EXPECT_TRUE(volume_archive->Cleanup());
  EXPECT_EQ(NULL, volume_archive->reader());

  // Cleanup is successful because archive_ was set to NULL by previous Cleanup
  // and archive_read_free will not be called in this case.
  fake_lib_archive_config::fail_archive_read_free = true;
  EXPECT_TRUE(volume_archive->Cleanup());
  EXPECT_EQ(NULL, volume_archive->reader());
}

TEST_F(VolumeArchiveLibarchiveTest, CleanupAfterInitFailure) {
  EXPECT_TRUE(volume_archive->reader() != NULL);

  fake_lib_archive_config::fail_archive_read_open = true;
  EXPECT_TRUE(!volume_archive->Init(kEncoding));

  // Test Cleanup after Init failure.
  EXPECT_TRUE(volume_archive->Cleanup());
  EXPECT_EQ(NULL, volume_archive->reader());
}
