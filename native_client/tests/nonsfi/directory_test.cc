/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include <algorithm>
#include <string>
#include <vector>

#include "native_client/src/include/nacl_assert.h"

void test_dirent(const char *test_directory) {
  DIR *dir;
  int rc;

  // Create files: test00, test01, test02 ... and test29 into the directory.
  const int kNumTestFiles = 30;
  {
    char path[PATH_MAX];
    for (int i = 0; i < kNumTestFiles; ++i) {
      snprintf(path, PATH_MAX, "%s/test%02d", test_directory, i);
      fclose(fopen(path, "w"));
    }
  }

  // Test readdir()
  {
    int dirfd = open(test_directory, O_RDONLY);
    ASSERT_GE(dirfd, 0);
    dir = fdopendir(dirfd);  // Takes ownership of |dirfd|.

    // We expect '.', '..', 'test00', 'test01', ... and 'test29' as entries.
    std::vector<std::string> paths;
    for (int i = 0; i < kNumTestFiles + 2; ++i) {
      struct dirent *result = readdir(dir);
      ASSERT_EQ(errno, 0);
      ASSERT_NE(result, NULL);
      ASSERT_NE(result->d_ino, 0);
      ASSERT_GT(result->d_reclen, 0);
      paths.push_back(std::string(result->d_name));
    }

    std::sort(paths.begin(), paths.end());
    ASSERT(paths[0] == ".");
    ASSERT(paths[1] == "..");
    for (int i = 0; i < kNumTestFiles; ++i) {
      char name[PATH_MAX];
      snprintf(name, PATH_MAX, "test%02d", i);
      ASSERT_MSG(paths[i + 2] == name, name);
    }

    // Here DIR should hit end of entries.
    {
      struct dirent *result = readdir(dir);
      ASSERT_EQ(errno, 0);
      ASSERT_EQ(result, NULL);
    }

    rc = closedir(dir);
    ASSERT_EQ(rc, 0);
  }

  // Test readdir_r()
  {
    int dirfd = open(test_directory, O_RDONLY);
    ASSERT_GE(dirfd, 0);
    dir = fdopendir(dirfd);  // Takes ownership of |dirfd|.

    // We expect '.', '..', 'test00', 'test01', ... and 'test29' as entries.
    std::vector<std::string> paths;
    for (int i = 0; i < kNumTestFiles + 2; ++i) {
      struct dirent entry;
      struct dirent *result;
      int rc = readdir_r(dir, &entry, &result);
      ASSERT_EQ(rc, 0);
      ASSERT_EQ(&entry, result);
      ASSERT_NE(result->d_ino, 0);
      ASSERT_GT(result->d_reclen, 0);
      paths.push_back(std::string(result->d_name));
    }

    std::sort(paths.begin(), paths.end());
    ASSERT(paths[0] == ".");
    ASSERT(paths[1] == "..");
    for (int i = 0; i < kNumTestFiles; ++i) {
      char name[PATH_MAX];
      snprintf(name, PATH_MAX, "test%02d", i);
      ASSERT_MSG(paths[i + 2] == name, name);
    }

    // Here DIR should hit end of entries.
    {
      struct dirent entry;
      struct dirent *result;
      int rc = readdir_r(dir, &entry, &result);
      ASSERT_EQ(rc, 0);
      ASSERT_EQ(result, NULL);
    }

    rc = closedir(dir);
    ASSERT_EQ(rc, 0);
  }

  // Test with bad file descriptor.
  dir = fdopendir(-1);
  ASSERT_EQ(dir, NULL);
  ASSERT_EQ(errno, EBADF);

  // Test with non-directory descriptor.
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "%s/test01", test_directory);
  int filefd = open(path, O_RDONLY);
  ASSERT_GE(filefd, 0);
  dir = fdopendir(filefd);
  ASSERT_EQ(dir, NULL);
  ASSERT_EQ(errno, ENOTDIR);

  rc = close(filefd);
  ASSERT_EQ(rc, 0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Please specify the test directory name.\n");
    exit(-1);
  }

  test_dirent(argv[1]);
  puts("PASSED");
  return 0;
}
