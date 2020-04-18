// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/stability_paths.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/process/process.h"
#include "base/test/multiprocess_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace browser_watcher {

class StabilityPathsMultiProcTest : public base::MultiProcessTest {};

MULTIPROCESS_TEST_MAIN(DummyProcess) {
  return 0;
}

TEST_F(StabilityPathsMultiProcTest, GetStabilityFileForProcessTest) {
  const base::FilePath empty_path;

  // Get the path for the current process.
  base::FilePath stability_path;
  ASSERT_TRUE(GetStabilityFileForProcess(base::Process::Current(), empty_path,
                                         &stability_path));

  // Ensure requesting a second time produces the same.
  base::FilePath stability_path_two;
  ASSERT_TRUE(GetStabilityFileForProcess(base::Process::Current(), empty_path,
                                         &stability_path_two));
  EXPECT_EQ(stability_path, stability_path_two);

  // Ensure a different process has a different stability path.
  base::Process process = SpawnChild("DummyProcess");
  base::FilePath stability_path_other;
  ASSERT_TRUE(
      GetStabilityFileForProcess(process, empty_path, &stability_path_other));
  EXPECT_NE(stability_path, stability_path_other);
}

TEST(StabilityPathsTest,
     GetStabilityFilePatternMatchesGetStabilityFileForProcessResult) {
  // GetStabilityFileForProcess file names must match GetStabilityFilePattern
  // according to
  // FileEnumerator's algorithm. We test this by writing out some files and
  // validating what is matched.

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath user_data_dir = temp_dir.GetPath();

  // Create the stability directory.
  base::FilePath stability_dir = GetStabilityDir(user_data_dir);
  ASSERT_TRUE(base::CreateDirectory(stability_dir));

  // Write a stability file.
  base::FilePath stability_file;
  ASSERT_TRUE(GetStabilityFileForProcess(base::Process::Current(),
                                         user_data_dir, &stability_file));
  {
    base::ScopedFILE file(base::OpenFile(stability_file, "w"));
    ASSERT_TRUE(file.get());
  }

  // Write a file that shouldn't match.
  base::FilePath non_matching_file =
      stability_dir.AppendASCII("non_matching.foo");
  {
    base::ScopedFILE file(base::OpenFile(non_matching_file, "w"));
    ASSERT_TRUE(file.get());
  }

  // Validate only the stability file matches.
  base::FileEnumerator enumerator(stability_dir, false /* recursive */,
                                  base::FileEnumerator::FILES,
                                  GetStabilityFilePattern());
  ASSERT_EQ(stability_file, enumerator.Next());
  ASSERT_TRUE(enumerator.Next().empty());
}

TEST(StabilityPathsTest, GetStabilityFiles) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  // Create files.
  std::vector<base::FilePath> expected_paths;
  std::set<base::FilePath> excluded_paths;
  {
    // Matches the pattern.
    base::FilePath path = temp_dir.GetPath().AppendASCII("foo1.pma");
    base::ScopedFILE file(base::OpenFile(path, "w"));
    ASSERT_NE(file.get(), nullptr);
    expected_paths.push_back(path);

    // Matches the pattern, but is excluded.
    path = temp_dir.GetPath().AppendASCII("foo2.pma");
    file.reset(base::OpenFile(path, "w"));
    ASSERT_NE(file.get(), nullptr);
    ASSERT_TRUE(excluded_paths.insert(path).second);

    // Matches the pattern.
    path = temp_dir.GetPath().AppendASCII("foo3.pma");
    file.reset(base::OpenFile(path, "w"));
    ASSERT_NE(file.get(), nullptr);
    expected_paths.push_back(path);

    // Does not match the pattern.
    path = temp_dir.GetPath().AppendASCII("bar.baz");
    file.reset(base::OpenFile(path, "w"));
    ASSERT_NE(file.get(), nullptr);
  }

  EXPECT_THAT(GetStabilityFiles(temp_dir.GetPath(),
                                FILE_PATH_LITERAL("foo*.pma"), excluded_paths),
              testing::UnorderedElementsAreArray(expected_paths));
}

}  // namespace browser_watcher
