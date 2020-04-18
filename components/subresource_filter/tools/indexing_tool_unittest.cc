// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/tools/indexing_tool.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "components/subresource_filter/core/common/test_ruleset_creator.h"
#include "components/subresource_filter/core/common/test_ruleset_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace subresource_filter {

namespace proto = url_pattern_index::proto;

namespace {

std::vector<uint8_t> ReadFileContents(const base::FilePath& file_path) {
  base::File file(file_path, base::File::FLAG_OPEN | base::File::FLAG_READ);

  size_t length = base::checked_cast<size_t>(file.GetLength());
  std::vector<uint8_t> contents(length);
  static_assert(sizeof(uint8_t) == sizeof(char), "Expected char = byte.");
  file.Read(0, reinterpret_cast<char*>(contents.data()),
            base::checked_cast<int>(length));
  return contents;
}

class IndexingToolTest : public ::testing::Test {
 public:
  IndexingToolTest() {}

 protected:
  void SetUp() override { ASSERT_TRUE(scoped_temp_dir_.CreateUniqueTempDir()); }

  base::FilePath GetUniquePath() {
    base::FilePath path = scoped_temp_dir_.GetPath().AppendASCII(
        base::IntToString(file_count_++));
    return path;
  }

  void CreateSimpleRuleset() {
    std::vector<proto::UrlRule> rules;
    rules.push_back(testing::CreateSuffixRule("disallowed1.png"));
    rules.push_back(testing::CreateSuffixRule("disallowed2.png"));
    rules.push_back(testing::CreateSuffixRule("disallowed3.png"));
    rules.push_back(
        testing::CreateWhitelistSuffixRule("whitelist/disallowed1.png"));
    rules.push_back(
        testing::CreateWhitelistSuffixRule("whitelist/disallowed2.png"));

    ASSERT_NO_FATAL_FAILURE(test_ruleset_creator_.CreateRulesetWithRules(
        rules, &test_ruleset_pair_));
  }

  void WriteUnindexedRulesetToFile(const base::FilePath& path) {
    // Write the test unindexed data to a file.
    const std::vector<uint8_t>& unindexed_data =
        test_ruleset_pair_.unindexed.contents;
    base::WriteFile(path, reinterpret_cast<const char*>(unindexed_data.data()),
                    base::checked_cast<int>(unindexed_data.size()));
  }

  int file_count_ = 0;
  base::ScopedTempDir scoped_temp_dir_;
  testing::TestRulesetCreator test_ruleset_creator_;
  testing::TestRulesetPair test_ruleset_pair_;

  DISALLOW_COPY_AND_ASSIGN(IndexingToolTest);
};

TEST_F(IndexingToolTest, UnindexedFileDoesNotExist) {
  // There is no file at the unindexed position, so it should return false.
  EXPECT_FALSE(IndexAndWriteRuleset(GetUniquePath(), GetUniquePath()));
}

TEST_F(IndexingToolTest, IndexedDirectoryDoesNotExist) {
  // Create a valid unindexed file.
  base::FilePath unindexed_path = GetUniquePath();
  CreateSimpleRuleset();
  WriteUnindexedRulesetToFile(unindexed_path);

  // The indexed path is in a non-existant directory.
  base::FilePath indexed_path =
      scoped_temp_dir_.GetPath().AppendASCII("foo/bar");

  // This should fail because the directory for indexed_path doesn't exist.
  EXPECT_FALSE(IndexAndWriteRuleset(unindexed_path, indexed_path));
}

TEST_F(IndexingToolTest, VerifyOutput) {
  base::FilePath unindexed_path = GetUniquePath();
  base::FilePath indexed_path = GetUniquePath();

  CreateSimpleRuleset();
  WriteUnindexedRulesetToFile(unindexed_path);

  // Convert the unindexed data to indexed data, and write the result to
  // indexed_path.
  EXPECT_TRUE(IndexAndWriteRuleset(unindexed_path, indexed_path));

  // Verify that the output equals the test indexed data.
  std::vector<uint8_t> indexed_data = ReadFileContents(indexed_path);
  EXPECT_EQ(test_ruleset_pair_.indexed.contents, indexed_data);
}

}  // namespace

}  // namespace subresource_filter
