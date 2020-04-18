// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "components/services/leveldb/leveldb_struct_traits.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/leveldb_chrome.h"

namespace {

class LevelDBServiceMojoTest : public testing::Test {};

}  // namespace

TEST(LevelDBServiceMojoTest, TestSerialization) {
  leveldb_env::Options input;
  // Tweak all input values to have a non-default value.
  input.create_if_missing = !input.create_if_missing;
  input.error_if_exists = !input.error_if_exists;
  input.paranoid_checks = !input.paranoid_checks;
  input.write_buffer_size += 1;
  input.max_open_files += 1;
  input.block_cache = leveldb_chrome::GetSharedWebBlockCache();

  leveldb_env::Options output;
  ASSERT_TRUE(leveldb::mojom::OpenOptions::Deserialize(
      leveldb::mojom::OpenOptions::Serialize(&input), &output));

  EXPECT_EQ(output.create_if_missing, output.create_if_missing);
  EXPECT_EQ(output.error_if_exists, output.error_if_exists);
  EXPECT_EQ(output.paranoid_checks, output.paranoid_checks);
  EXPECT_EQ(output.write_buffer_size, output.write_buffer_size);
  EXPECT_EQ(output.max_open_files, output.max_open_files);
  EXPECT_EQ(leveldb_chrome::GetSharedWebBlockCache(), output.block_cache);
}
