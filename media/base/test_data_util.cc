// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/test_data_util.h"

#include <stdint.h>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/path_service.h"
#include "media/base/decoder_buffer.h"

namespace media {

namespace {

// Key used to encrypt test files.
const uint8_t kSecretKey[] = {0xeb, 0xdd, 0x62, 0xf1, 0x68, 0x14, 0xd2, 0x7b,
                              0x68, 0xef, 0x12, 0x2a, 0xfc, 0xe4, 0xae, 0x3c};

// The key ID for all encrypted files.
const uint8_t kKeyId[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                          0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35};
}

// TODO(sandersd): Change the tests to use a more unique message.
// See http://crbug.com/592067

// Common test results.
const char kFailed[] = "FAILED";

// Upper case event name set by Utils.installTitleEventHandler().
const char kEnded[] = "ENDED";
const char kErrorEvent[] = "ERROR";

// Lower case event name as set by Utils.failTest().
const char kError[] = "error";

const base::FilePath::CharType kTestDataPath[] =
    FILE_PATH_LITERAL("media/test/data");

base::FilePath GetTestDataFilePath(const std::string& name) {
  base::FilePath file_path;
  CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &file_path));
  return file_path.Append(GetTestDataPath()).AppendASCII(name);
}

base::FilePath GetTestDataPath() {
  return base::FilePath(kTestDataPath);
}

std::string GetURLQueryString(const base::StringPairs& query_params) {
  std::string query = "";
  base::StringPairs::const_iterator itr = query_params.begin();
  for (; itr != query_params.end(); ++itr) {
    if (itr != query_params.begin())
      query.append("&");
    query.append(itr->first + "=" + itr->second);
  }
  return query;
}

scoped_refptr<DecoderBuffer> ReadTestDataFile(const std::string& name) {
  base::FilePath file_path = GetTestDataFilePath(name);

  int64_t tmp = 0;
  CHECK(base::GetFileSize(file_path, &tmp))
      << "Failed to get file size for '" << name << "'";

  int file_size = base::checked_cast<int>(tmp);

  scoped_refptr<DecoderBuffer> buffer(new DecoderBuffer(file_size));
  CHECK_EQ(file_size,
           base::ReadFile(
               file_path, reinterpret_cast<char*>(buffer->writable_data()),
               file_size)) << "Failed to read '" << name << "'";

  return buffer;
}

bool LookupTestKeyVector(const std::vector<uint8_t>& key_id,
                         bool allow_rotation,
                         std::vector<uint8_t>* key) {
  std::vector<uint8_t> starting_key_id(kKeyId, kKeyId + arraysize(kKeyId));
  size_t rotate_limit = allow_rotation ? starting_key_id.size() : 1;
  for (size_t pos = 0; pos < rotate_limit; ++pos) {
    std::rotate(starting_key_id.begin(), starting_key_id.begin() + pos,
                starting_key_id.end());
    if (key_id == starting_key_id) {
      key->assign(kSecretKey, kSecretKey + arraysize(kSecretKey));
      std::rotate(key->begin(), key->begin() + pos, key->end());
      return true;
    }
  }
  return false;
}

bool LookupTestKeyString(const std::string& key_id,
                         bool allow_rotation,
                         std::string* key) {
  std::vector<uint8_t> key_vector;
  bool result =
      LookupTestKeyVector(std::vector<uint8_t>(key_id.begin(), key_id.end()),
                          allow_rotation, &key_vector);
  if (result)
    *key = std::string(key_vector.begin(), key_vector.end());
  return result;
}

}  // namespace media
