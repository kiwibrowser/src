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

// Fake archive data used for testing.
const char kArchiveData[] =
    "Fake data contained by the archive. Content is "
    "not important and it is used strictly for testing.";

}  // namespace

// Class used by TEST_F macro to initialize the environment for testing
// VolumeArchiveLibarchive Read method.
class VolumeArchiveLibarchiveReadTest : public testing::Test {
 protected:
  VolumeArchiveLibarchiveReadTest() : volume_archive(NULL) {}

  virtual void SetUp() {
    fake_lib_archive_config::ResetVariables();
    // Pass FakeVolumeReader ownership to VolumeArchiveLibarchive.
    volume_archive = new VolumeArchiveLibarchive(new FakeVolumeReader());

    // Prepare for read.
    ASSERT_TRUE(volume_archive->Init(kEncoding));
    const char* path_name = NULL;
    int64_t size = 0;
    bool is_directory = false;
    time_t modification_time = 0;
    ASSERT_EQ(VolumeArchive::RESULT_SUCCESS,
              volume_archive->GetNextHeader(&path_name, &size, &is_directory,
                                            &modification_time));
  }

  virtual void TearDown() {
    volume_archive->Cleanup();
    delete volume_archive;
    volume_archive = NULL;
  }

  VolumeArchiveLibarchive* volume_archive;
};

// Test successful ReadData with length equal to data size.
TEST_F(VolumeArchiveLibarchiveReadTest, ReadSuccessAllData) {
  fake_lib_archive_config::archive_data = kArchiveData;
  fake_lib_archive_config::archive_data_size = sizeof(kArchiveData);
  int64_t archive_data_size = fake_lib_archive_config::archive_data_size;

  int64_t length = archive_data_size;
  const char* buffer = NULL;
  int64_t read_bytes = volume_archive->ReadData(0, length, &buffer);
  EXPECT_LT(0, read_bytes);
  EXPECT_GE(length, read_bytes);
  EXPECT_EQ(0, memcmp(buffer, kArchiveData, read_bytes));
}

// This test is used to test VolumeArchive::ReadData for correct reads with
// different offsets, lengths and a buffer that has different characters inside.
// Tests lengths < volume_archive_constants::kMininumDataChunkSize.
// VolumeArchive::ReadData should not be affected by this constant.
TEST_F(VolumeArchiveLibarchiveReadTest, ReadSuccessForSmallLengths) {
  fake_lib_archive_config::archive_data = kArchiveData;
  fake_lib_archive_config::archive_data_size = sizeof(kArchiveData);
  int64_t archive_data_size = fake_lib_archive_config::archive_data_size;

  // Test successful read with offset less than VolumeArchiveLibarchive current
  // offset (due to last read) and length equal to 1/3 of the data size.
  {
    int64_t length = archive_data_size / 3;
    const char* buffer = NULL;
    int64_t read_bytes = volume_archive->ReadData(0, length, &buffer);
    EXPECT_LT(0, read_bytes);
    EXPECT_GE(length, read_bytes);
    EXPECT_EQ(0, memcmp(buffer, kArchiveData, read_bytes));
  }

  // Test successful read for the next 1/3 chunk.
  {
    int64_t offset = archive_data_size / 3;
    int64_t length = archive_data_size / 3;
    const char* buffer = NULL;
    int64_t read_bytes = volume_archive->ReadData(offset, length, &buffer);
    EXPECT_LT(0, read_bytes);
    EXPECT_GE(length, read_bytes);
    EXPECT_EQ(0, memcmp(buffer, kArchiveData + offset, read_bytes));
  }

  // Test successful read for the last 1/3 chunk.
  {
    int64_t offset = archive_data_size / 3 * 2;
    int64_t length = archive_data_size - offset;
    const char* buffer = NULL;
    int64_t read_bytes = volume_archive->ReadData(offset, length, &buffer);
    EXPECT_LT(0, read_bytes);
    EXPECT_GE(length, read_bytes);
    EXPECT_EQ(0, memcmp(buffer, kArchiveData + offset, read_bytes));
  }
}

// Similar to above, just that this time we also call
// VolumeArchiveLibarchive::volume_archive->MaybeDecompressAhead to improve
// perfomance.
TEST_F(VolumeArchiveLibarchiveReadTest, ReadSuccessForSmallLengthsWithConsume) {
  fake_lib_archive_config::archive_data = kArchiveData;
  fake_lib_archive_config::archive_data_size = sizeof(kArchiveData);
  int64_t archive_data_size = fake_lib_archive_config::archive_data_size;

  // Test successful read with offset less than VolumeArchiveLibarchive current
  // offset (due to last read) and length equal to 1/3 of the data size.
  {
    int64_t length = archive_data_size / 3;
    const char* buffer = NULL;
    int64_t read_bytes = volume_archive->ReadData(0, length, &buffer);
    EXPECT_LT(0, read_bytes);
    EXPECT_GE(length, read_bytes);
    EXPECT_EQ(0, memcmp(buffer, kArchiveData, read_bytes));
    volume_archive->MaybeDecompressAhead();
  }

  // Test successful read for the next 1/3 chunk.
  {
    int64_t offset = archive_data_size / 3;
    int64_t length = archive_data_size / 3;
    const char* buffer = NULL;
    int64_t read_bytes = volume_archive->ReadData(offset, length, &buffer);
    EXPECT_LT(0, read_bytes);
    EXPECT_GE(length, read_bytes);
    EXPECT_EQ(0, memcmp(buffer, kArchiveData + offset, read_bytes));
    volume_archive->MaybeDecompressAhead();
  }

  // Test successful read for the last 1/3 chunk.
  {
    int64_t offset = archive_data_size / 3 * 2;
    int64_t length = archive_data_size - offset;
    const char* buffer = NULL;
    int64_t read_bytes = volume_archive->ReadData(offset, length, &buffer);
    EXPECT_LT(0, read_bytes);
    EXPECT_GE(length, read_bytes);
    EXPECT_EQ(0, memcmp(buffer, kArchiveData + offset, read_bytes));
    volume_archive->MaybeDecompressAhead();
  }
}

// Test read with length greater than data size.
TEST_F(VolumeArchiveLibarchiveReadTest,
       ReadSuccessForSmallLengthGreaterThanArchiveDataSize) {
  fake_lib_archive_config::archive_data = kArchiveData;
  fake_lib_archive_config::archive_data_size = sizeof(kArchiveData);
  int64_t archive_data_size = fake_lib_archive_config::archive_data_size;

  int64_t length = archive_data_size * 2;
  const char* buffer = NULL;
  int64_t read_bytes = volume_archive->ReadData(0, length, &buffer);
  EXPECT_LT(0, read_bytes);
  EXPECT_GE(archive_data_size, read_bytes);
  EXPECT_EQ(0, memcmp(buffer, kArchiveData, read_bytes));
}

// Test Read with length between volume_archive_constants::kMinimumDataChunkSize
// and volume_archive_constants::kMaximumDataChunkSize.
// VolumeArchive::ReadData should not be affected by this constant.
TEST_F(VolumeArchiveLibarchiveReadTest, ReadSuccessForMediumLength) {
  int64_t buffer_length = volume_archive_constants::kMinimumDataChunkSize * 2;
  ASSERT_LT(buffer_length, volume_archive_constants::kMaximumDataChunkSize);

  char* expected_buffer = new char[buffer_length];  // Stack is small for tests.
  memset(expected_buffer, 1, buffer_length);

  fake_lib_archive_config::archive_data = expected_buffer;
  fake_lib_archive_config::archive_data_size = buffer_length;

  const char* buffer = NULL;
  int64_t read_bytes = volume_archive->ReadData(0, buffer_length, &buffer);
  EXPECT_LT(0, read_bytes);
  EXPECT_GE(buffer_length, read_bytes);
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, read_bytes));

  delete expected_buffer;
}

// Test Read with length > volume_archive_constants::kMaximumDataChunkSize.
// VolumeArchive::ReadData should not be affected by this constant.
TEST_F(VolumeArchiveLibarchiveReadTest, ReadSuccessForLargeLength) {
  int64_t buffer_length = volume_archive_constants::kMaximumDataChunkSize * 2;

  char* expected_buffer = new char[buffer_length];  // Stack is small for tests.
  memset(expected_buffer, 1, buffer_length);

  fake_lib_archive_config::archive_data = expected_buffer;
  fake_lib_archive_config::archive_data_size = buffer_length;

  const char* buffer = NULL;
  int64_t read_bytes = volume_archive->ReadData(0, buffer_length, &buffer);
  EXPECT_LT(0, read_bytes);
  EXPECT_GE(buffer_length, read_bytes);
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, read_bytes));

  delete expected_buffer;
}

TEST_F(VolumeArchiveLibarchiveReadTest, ReadFailureForOffsetEqualToZero) {
  fake_lib_archive_config::archive_data = NULL;
  const char* buffer;
  EXPECT_GT(0, volume_archive->ReadData(0, 10, &buffer));

  std::string read_data_error =
      std::string(volume_archive_constants::kArchiveReadDataErrorPrefix) +
      fake_lib_archive_config::kArchiveError;
  EXPECT_EQ(read_data_error, volume_archive->error_message());
}

TEST_F(VolumeArchiveLibarchiveReadTest, ReadFailureForOffsetGreaterThanZero) {
  fake_lib_archive_config::archive_data = NULL;
  const char* buffer;
  EXPECT_GT(0, volume_archive->ReadData(0, 10, &buffer));

  std::string read_data_error =
      std::string(volume_archive_constants::kArchiveReadDataErrorPrefix) +
      fake_lib_archive_config::kArchiveError;
  EXPECT_EQ(read_data_error, volume_archive->error_message());
}

TEST_F(VolumeArchiveLibarchiveReadTest, ReadFailureAfterSucessfulRead) {
  // Read successfully the first chunk.
  fake_lib_archive_config::archive_data = kArchiveData;
  fake_lib_archive_config::archive_data_size = sizeof(kArchiveData);
  int64_t archive_data_size = fake_lib_archive_config::archive_data_size;
  {
    int64_t length = archive_data_size / 2;
    const char* buffer = NULL;
    int64_t read_bytes = volume_archive->ReadData(0, length, &buffer);
    EXPECT_LT(0, read_bytes);
    EXPECT_GE(length, read_bytes);
    EXPECT_EQ(0, memcmp(buffer, kArchiveData, read_bytes));
  }

  // Force failure on the second chunk.
  fake_lib_archive_config::archive_data = NULL;
  {
    int64_t offset = archive_data_size / 2;
    int64_t length = archive_data_size - offset;
    const char* buffer = NULL;
    EXPECT_GT(0, volume_archive->ReadData(offset, length, &buffer));

    std::string read_data_error =
        std::string(volume_archive_constants::kArchiveReadDataErrorPrefix) +
        fake_lib_archive_config::kArchiveError;
    EXPECT_EQ(read_data_error, volume_archive->error_message());
  }
}
