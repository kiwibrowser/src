// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "request.h"

#include <limits>
#include <sstream>

#include "testing/gtest/gtest.h"

namespace {

const char kFileSystemId[] = "id";
const char kRequestId[] = "0";
const char kError[] = "error";
const int64_t kLength = 100;

}  // namespace

TEST(request, CreateReadMetadataDoneResponse) {
  pp::VarDictionary metadata;
  metadata.Set("/", "Everything is fine.");

  pp::VarDictionary metadata_done = request::CreateReadMetadataDoneResponse(
      kFileSystemId, kRequestId, metadata);

  EXPECT_TRUE(metadata_done.Get(request::key::kOperation).is_int());
  EXPECT_EQ(request::READ_METADATA_DONE,
            metadata_done.Get(request::key::kOperation).AsInt());

  EXPECT_TRUE(metadata_done.Get(request::key::kFileSystemId).is_string());
  EXPECT_EQ(kFileSystemId,
            metadata_done.Get(request::key::kFileSystemId).AsString());

  EXPECT_TRUE(metadata_done.Get(request::key::kRequestId).is_string());
  EXPECT_EQ(kRequestId, metadata_done.Get(request::key::kRequestId).AsString());

  EXPECT_TRUE(metadata_done.Get(request::key::kMetadata).is_dictionary());
  EXPECT_EQ(metadata,
            pp::VarDictionary(metadata_done.Get(request::key::kMetadata)));
}

TEST(request, CreateReadChunkRequest) {
  int64_t expected_offset = std::numeric_limits<int64_t>::max();
  pp::VarDictionary read_chunk = request::CreateReadChunkRequest(
      kFileSystemId, kRequestId, expected_offset, kLength);

  EXPECT_TRUE(read_chunk.Get(request::key::kOperation).is_int());
  EXPECT_EQ(request::READ_CHUNK,
            read_chunk.Get(request::key::kOperation).AsInt());

  EXPECT_TRUE(read_chunk.Get(request::key::kFileSystemId).is_string());
  EXPECT_EQ(kFileSystemId,
            read_chunk.Get(request::key::kFileSystemId).AsString());

  EXPECT_TRUE(read_chunk.Get(request::key::kRequestId).is_string());
  EXPECT_EQ(kRequestId, read_chunk.Get(request::key::kRequestId).AsString());

  EXPECT_TRUE(read_chunk.Get(request::key::kOffset).is_string());
  std::stringstream ss_offset(read_chunk.Get(request::key::kOffset).AsString());
  int64_t offset;
  ss_offset >> offset;
  EXPECT_EQ(expected_offset, offset);

  EXPECT_TRUE(read_chunk.Get(request::key::kLength).is_string());
  std::stringstream ss_length(read_chunk.Get(request::key::kLength).AsString());
  int64_t length;
  ss_length >> length;
  EXPECT_EQ(kLength, length);
}

TEST(request, CreateOpenFileDoneResponse) {
  pp::VarDictionary open_file_done =
      request::CreateOpenFileDoneResponse(kFileSystemId, kRequestId);

  EXPECT_TRUE(open_file_done.Get(request::key::kOperation).is_int());
  EXPECT_EQ(request::OPEN_FILE_DONE,
            open_file_done.Get(request::key::kOperation).AsInt());

  EXPECT_TRUE(open_file_done.Get(request::key::kFileSystemId).is_string());
  EXPECT_EQ(kFileSystemId,
            open_file_done.Get(request::key::kFileSystemId).AsString());

  EXPECT_TRUE(open_file_done.Get(request::key::kRequestId).is_string());
  EXPECT_EQ(kRequestId,
            open_file_done.Get(request::key::kRequestId).AsString());
}

TEST(request, CreateCloseFileDoneResponse) {
  std::string open_request_id = "1";
  pp::VarDictionary close_file_done = request::CreateCloseFileDoneResponse(
      kFileSystemId, kRequestId, open_request_id);

  EXPECT_TRUE(close_file_done.Get(request::key::kOperation).is_int());
  EXPECT_EQ(request::CLOSE_FILE_DONE,
            close_file_done.Get(request::key::kOperation).AsInt());

  EXPECT_TRUE(close_file_done.Get(request::key::kFileSystemId).is_string());
  EXPECT_EQ(kFileSystemId,
            close_file_done.Get(request::key::kFileSystemId).AsString());

  EXPECT_TRUE(close_file_done.Get(request::key::kRequestId).is_string());
  EXPECT_EQ(kRequestId,
            close_file_done.Get(request::key::kRequestId).AsString());

  EXPECT_TRUE(close_file_done.Get(request::key::kOpenRequestId).is_string());
  EXPECT_EQ(open_request_id,
            close_file_done.Get(request::key::kOpenRequestId).AsString());
}

TEST(request, CreateReadFileDoneResponse) {
  pp::VarArrayBuffer array_buffer(50);
  bool has_more_data = true;

  pp::VarDictionary read_file_done = request::CreateReadFileDoneResponse(
      kFileSystemId, kRequestId, array_buffer, has_more_data);

  EXPECT_TRUE(read_file_done.Get(request::key::kOperation).is_int());
  EXPECT_EQ(request::READ_FILE_DONE,
            read_file_done.Get(request::key::kOperation).AsInt());

  EXPECT_TRUE(read_file_done.Get(request::key::kFileSystemId).is_string());
  EXPECT_EQ(kFileSystemId,
            read_file_done.Get(request::key::kFileSystemId).AsString());

  EXPECT_TRUE(read_file_done.Get(request::key::kRequestId).is_string());
  EXPECT_EQ(kRequestId,
            read_file_done.Get(request::key::kRequestId).AsString());

  EXPECT_TRUE(
      read_file_done.Get(request::key::kReadFileData).is_array_buffer());
  EXPECT_EQ(array_buffer, pp::VarArrayBuffer(
                              read_file_done.Get(request::key::kReadFileData)));

  EXPECT_TRUE(read_file_done.Get(request::key::kHasMoreData).is_bool());
  EXPECT_EQ(has_more_data,
            read_file_done.Get(request::key::kHasMoreData).AsBool());
}

TEST(request, CreateFileSystemError) {
  pp::VarDictionary error =
      request::CreateFileSystemError(kFileSystemId, kRequestId, kError);

  EXPECT_TRUE(error.Get(request::key::kOperation).is_int());
  EXPECT_EQ(request::FILE_SYSTEM_ERROR,
            error.Get(request::key::kOperation).AsInt());

  EXPECT_TRUE(error.Get(request::key::kFileSystemId).is_string());
  EXPECT_EQ(kFileSystemId, error.Get(request::key::kFileSystemId).AsString());

  EXPECT_TRUE(error.Get(request::key::kRequestId).is_string());
  EXPECT_EQ(kRequestId, error.Get(request::key::kRequestId).AsString());

  EXPECT_TRUE(error.Get(request::key::kError).is_string());
  EXPECT_EQ(kError, error.Get(request::key::kError).AsString());
}
