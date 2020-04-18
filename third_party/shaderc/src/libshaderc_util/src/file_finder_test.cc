// Copyright 2015 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "libshaderc_util/file_finder.h"

#include <gtest/gtest.h>

#include "death_test.h"

namespace {

using shaderc_util::FileFinder;

const std::string kCurrentDir = CURRENT_DIR;  // A macro set by cmake.

class FileFinderTest : public testing::Test {
 protected:
  FileFinder finder;
};

TEST_F(FileFinderTest, PathStartsEmpty) {
  EXPECT_TRUE(FileFinder().search_path().empty());
}

TEST_F(FileFinderTest, EmptyPath) {
  finder.search_path().clear();
  EXPECT_EQ("", finder.FindReadableFilepath("include_file.1"));
}

TEST_F(FileFinderTest, EmptyStringInPath) {
  finder.search_path() = {""};
  EXPECT_EQ("include_file.1", finder.FindReadableFilepath("include_file.1"));
  EXPECT_EQ("dir/subdir/include_file.2",
            finder.FindReadableFilepath("dir/subdir/include_file.2"));
}

TEST_F(FileFinderTest, SimplePath) {
  finder.search_path() = {"dir"};
  EXPECT_EQ("dir/subdir/include_file.2",
            finder.FindReadableFilepath("subdir/include_file.2"));
}

TEST_F(FileFinderTest, PathEndsInSlash) {
  finder.search_path() = {"dir/"};
  EXPECT_EQ("dir/subdir/include_file.2",
            finder.FindReadableFilepath("subdir/include_file.2"));
}

TEST_F(FileFinderTest, ParentDir) {
  finder.search_path() = {"dir"};
  EXPECT_EQ("dir/../include_file.1",
            finder.FindReadableFilepath("../include_file.1"));
}

TEST_F(FileFinderTest, EntirePathIsActive) {
  finder.search_path() = {"", "dir/subdir/"};
  EXPECT_EQ("include_file.1", finder.FindReadableFilepath("include_file.1"));
  EXPECT_EQ("dir/subdir/include_file.2",
            finder.FindReadableFilepath("include_file.2"));
}

TEST_F(FileFinderTest, NonExistingFile) {
  finder.search_path() = {"", "dir/subdir/"};
  EXPECT_EQ("", finder.FindReadableFilepath("garbage.xyxyxyxyxyxz"));
}

TEST_F(FileFinderTest, FirstHitReturned) {
  finder.search_path() = {".", "", "dir/../"};
  EXPECT_EQ("./include_file.1", finder.FindReadableFilepath("include_file.1"));
}

TEST_F(FileFinderTest, IrrelevantPaths) {
  finder.search_path() = {".", "garbage.xyxyxyxyxyz", "dir/../"};
  EXPECT_EQ("", finder.FindReadableFilepath("include_file.2"));
  finder.search_path().push_back("dir/subdir");
  EXPECT_EQ("dir/subdir/include_file.2",
            finder.FindReadableFilepath("include_file.2"));
}

TEST_F(FileFinderTest, CurrentDirectory) {
  ASSERT_GE(kCurrentDir.size(), 0u);
  // Either the directory should start with / (if we are on Linux),
  // Or it should beither X:/ or X:\ or // (if we are on Windows).
  ASSERT_TRUE(kCurrentDir.front() == '\\' || kCurrentDir.front() == '/' ||
    (kCurrentDir.size() >= 3u && kCurrentDir[1] == ':' &&
    (kCurrentDir[2] == '\\' || kCurrentDir[2] == '/')));
}

TEST_F(FileFinderTest, AbsolutePath) {
  ASSERT_NE('/', kCurrentDir.back());
  finder.search_path() = {kCurrentDir};
  EXPECT_EQ(kCurrentDir + "/include_file.1",
            finder.FindReadableFilepath("include_file.1"));
  EXPECT_EQ(kCurrentDir + "/dir/subdir/include_file.2",
            finder.FindReadableFilepath("dir/subdir/include_file.2"));
}

TEST_F(FileFinderTest, AbsoluteFilename) {
  ASSERT_NE('/', kCurrentDir.back());

  finder.search_path() = {""};
  const std::string absolute_file1 = kCurrentDir + "/include_file.1";
  EXPECT_EQ(absolute_file1, finder.FindReadableFilepath(absolute_file1));
  EXPECT_EQ("", finder.FindReadableFilepath("/dir/subdir/include_file.2"));

  finder.search_path().push_back(".");
  EXPECT_EQ(".//dir/subdir/include_file.2",
            finder.FindReadableFilepath("/dir/subdir/include_file.2"));
}

TEST(FileFinderDeathTest, EmptyFilename) {
  EXPECT_DEBUG_DEATH_IF_SUPPORTED(FileFinder().FindReadableFilepath(""),
                                  "Assertion");
}

}  // anonymous namespace
