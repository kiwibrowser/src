/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string>
#include <vector>

#include "native_client/src/trusted/service_runtime/filename_util.h"

#include "gtest/gtest.h"

class SelLdrFilesTest : public testing::Test {
};

void CheckCanonical(const std::string &abs_path, const std::string &goal_path,
                    const std::vector<std::string> &goal_subpaths) {
  std::string real_path;
  std::vector<std::string> required_subpaths;
  CanonicalizeAbsolutePath(abs_path, &real_path, &required_subpaths);
  ASSERT_STREQ(goal_path.c_str(), real_path.c_str());
  ASSERT_EQ(goal_subpaths.size(), required_subpaths.size());
  for (size_t i = 0; i != goal_subpaths.size(); i++) {
    ASSERT_STREQ(goal_subpaths[i].c_str(), required_subpaths[i].c_str());
  }
}

TEST_F(SelLdrFilesTest, TestModifiedRoot) {
  std::vector<std::string> goal_subpaths;
  goal_subpaths.clear();
  CheckCanonical("/foo", "/foo", goal_subpaths);
  CheckCanonical("/foo/", "/foo/", goal_subpaths);
  CheckCanonical("/foo/.", "/foo/", goal_subpaths);
  CheckCanonical("/foo/bar", "/foo/bar", goal_subpaths);

  CheckCanonical("//.", "/", goal_subpaths);
  CheckCanonical("///////", "/", goal_subpaths);
  CheckCanonical("//././/.////.///.././", "/", goal_subpaths);

  CheckCanonical("/../foo", "/foo", goal_subpaths);
  CheckCanonical("/..foo/", "/..foo/", goal_subpaths);
  CheckCanonical("/.foo/", "/.foo/", goal_subpaths);

  std::string tmp_foo[] = {"/foo/"};
  goal_subpaths.assign(tmp_foo, tmp_foo + 1);
  CheckCanonical("/foo/../bar", "/bar", goal_subpaths);
  CheckCanonical("/foo/..", "/", goal_subpaths);
  CheckCanonical("/../foo/.././bar/./", "/bar/", goal_subpaths);

  std::string tmp_bar_foo[] = {"/bar/foo/"};
  goal_subpaths.assign(tmp_bar_foo, tmp_bar_foo + 1);
  CheckCanonical("/bar/foo/..", "/bar", goal_subpaths);

  std::string tmp_foo_bar[] = {"/..foo/bar/"};
  goal_subpaths.assign(tmp_foo_bar, tmp_foo_bar + 1);
  CheckCanonical("/..foo/bar/..", "/..foo", goal_subpaths);

  std::string tmp_foo_and_bar[] = {"/foo/", "/bar/"};
  goal_subpaths.assign(tmp_foo_and_bar, tmp_foo_and_bar + 2);
  CheckCanonical("/foo/../bar/..", "/", goal_subpaths);
}
