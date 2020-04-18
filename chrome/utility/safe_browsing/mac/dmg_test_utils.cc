// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/safe_browsing/mac/dmg_test_utils.h"

#include <stddef.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {
namespace dmg {
namespace test {

void GetTestFile(const char* file_name, base::File* file) {
  base::FilePath test_data;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_GEN_TEST_DATA, &test_data));

  base::FilePath path = test_data.AppendASCII("chrome")
      .AppendASCII("safe_browsing_dmg")
      .AppendASCII(file_name);

  *file = base::File(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  ASSERT_TRUE(file->IsValid());
}

bool ReadEntireStream(ReadStream* stream, std::vector<uint8_t>* data) {
  DCHECK(data->empty());
  uint8_t buffer[1024];
  size_t bytes_read = 0;
  do {
    bytes_read = 0;

    if (!stream->Read(buffer, sizeof(buffer), &bytes_read))
      return false;

    data->insert(data->end(), buffer, &buffer[bytes_read]);
  } while (bytes_read != 0);

  return true;
}

}  // namespace test
}  // namespace dmg
}  // namespace safe_browsing
