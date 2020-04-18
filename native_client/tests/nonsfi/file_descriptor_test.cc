/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "native_client/src/include/nacl_assert.h"

// Reads the whole file content from fd. On failure, returns an empty string.
static std::string read_from_fd(int fd) {
  std::string content;

  const size_t kBufLen = 80;
  char buf[kBufLen];
  while (true) {
    ssize_t bytes_read = read(fd, buf, kBufLen);
    if (bytes_read < 0)
      return "";  // On error.

    if (bytes_read == 0)
      return content;  // EOF

    content.append(buf, bytes_read);
  }
}

void test_openat(const char *test_directory) {
  puts("test for openat()");
  const char kTestFileName[] = "test_openat_file.txt";
  const char kTestFileContent[] = "Hello, World.\n";
  const std::string kTestPath =
      std::string(test_directory) + "/" + kTestFileName;

  // Create a test file under the |test_directory|.
  {
    FILE *fp = fopen(kTestPath.c_str(), "w");
    ASSERT_NE(fp, NULL);
    fputs(kTestFileContent, fp);
    ASSERT_EQ(0, fclose(fp));
  }

  int dirfd = open(test_directory, O_RDONLY | O_DIRECTORY);
  ASSERT_GE(dirfd, 0);

  int fd = openat(dirfd, kTestFileName, O_RDONLY);
  ASSERT_GE(fd, 0);

  // Read the file content.
  {
    std::string content = read_from_fd(fd);
    ASSERT(content == kTestFileContent);
  }

  // Open the non-directory fd.
  int fd2 = openat(fd, kTestFileName, O_RDONLY);
  ASSERT_EQ(fd2, -1);
  ASSERT_EQ(errno, ENOTDIR);
  errno = 0;

  int rc = close(fd);
  ASSERT_EQ(rc, 0);

  // Test for AT_FDCWD.
  {
    char original_directory[PATH_MAX];
    ASSERT_NE(getcwd(original_directory, PATH_MAX), NULL);
    ASSERT_EQ(chdir(test_directory), 0);

    fd = openat(AT_FDCWD, kTestFileName, O_RDONLY);
    ASSERT_GE(fd, 0);
    std::string content = read_from_fd(fd);
    ASSERT(content == kTestFileContent);

    ASSERT_EQ(chdir(original_directory), 0);
  }

  // Test for non-existing file.
  fd = openat(dirfd, "non-existing-file", O_RDONLY);
  ASSERT_EQ(fd, -1);
  ASSERT_EQ(errno, ENOENT);
  errno = 0;

  rc = close(dirfd);
  ASSERT_EQ(rc, 0);

  // Test for invalid directory fd.
  fd = openat(-1, kTestFileName, O_RDONLY);
  ASSERT_EQ(fd, -1);
  ASSERT_EQ(errno, EBADF);
}

void test_fstatat(const char *test_directory) {
  puts("test for fstatat()");
  const char kTestFileName[] = "test_fstatat_file.txt";
  const std::string kTestPath =
      std::string(test_directory) + "/" + kTestFileName;

  // Create an empty file.
  {
    FILE *fp = fopen(kTestPath.c_str(), "w");
    ASSERT_NE(fp, NULL);
    ASSERT_EQ(0, fclose(fp));
  }

  struct stat buf;
  struct stat buf2;
  int rc = stat(kTestPath.c_str(), &buf);
  ASSERT_EQ(rc, 0);
  int dirfd = open(test_directory, O_RDONLY | O_DIRECTORY);
  ASSERT_GE(dirfd, 0);
  // Currently, no |flag| is defined. So, pass 0.
  rc = fstatat(dirfd, kTestFileName, &buf2, 0);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(buf.st_dev, buf2.st_dev);
  ASSERT_EQ(buf.st_mode, buf2.st_mode);
  ASSERT_EQ(buf.st_nlink, buf2.st_nlink);
  ASSERT_EQ(buf.st_uid, buf2.st_uid);
  ASSERT_EQ(buf.st_gid, buf2.st_gid);
  ASSERT_EQ(buf.st_rdev, buf2.st_rdev);
  ASSERT_EQ(buf.st_size, buf2.st_size);
  ASSERT_EQ(buf.st_blksize, buf2.st_blksize);
  ASSERT_EQ(buf.st_blocks, buf2.st_blocks);
  ASSERT_EQ(buf.st_atime, buf2.st_atime);
  ASSERT_EQ(buf.st_mtime, buf2.st_mtime);
  ASSERT_EQ(buf.st_ctime, buf2.st_ctime);

  // Test for AT_FDCWD.
  {
    char original_directory[PATH_MAX];
    ASSERT_NE(getcwd(original_directory, PATH_MAX), NULL);
    ASSERT_EQ(chdir(test_directory), 0);

    rc = fstatat(AT_FDCWD, kTestFileName, &buf2, 0);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(buf.st_dev, buf2.st_dev);
    ASSERT_EQ(buf.st_mode, buf2.st_mode);
    ASSERT_EQ(buf.st_nlink, buf2.st_nlink);
    ASSERT_EQ(buf.st_uid, buf2.st_uid);
    ASSERT_EQ(buf.st_gid, buf2.st_gid);
    ASSERT_EQ(buf.st_rdev, buf2.st_rdev);
    ASSERT_EQ(buf.st_size, buf2.st_size);
    ASSERT_EQ(buf.st_blksize, buf2.st_blksize);
    ASSERT_EQ(buf.st_blocks, buf2.st_blocks);
    ASSERT_EQ(buf.st_atime, buf2.st_atime);
    ASSERT_EQ(buf.st_mtime, buf2.st_mtime);
    ASSERT_EQ(buf.st_ctime, buf2.st_ctime);

    ASSERT_EQ(chdir(original_directory), 0);
  }

  // Test for non-existing file.
  rc = fstatat(dirfd, "not-existing-file", &buf, 0);
  ASSERT_EQ(rc, -1);
  ASSERT_EQ(errno, ENOENT);
  errno = 0;

  rc = close(dirfd);
  ASSERT_EQ(rc, 0);

  // With invalid file descriptor.
  rc = fstatat(-1, kTestFileName, &buf2, 0);
  ASSERT_EQ(rc, -1);
  ASSERT_EQ(errno, EBADF);
  errno = 0;

  // Test with non-directory file descriptor.
  int fd = open(kTestPath.c_str(), O_RDONLY);
  ASSERT_GE(fd, 0);
  rc = fstatat(fd, kTestFileName, &buf2, 0);
  ASSERT_EQ(rc, -1);
  ASSERT_EQ(errno, ENOTDIR);
  errno = 0;

  rc = close(fd);
  ASSERT_EQ(rc, 0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Please specify the test directory name.\n");
    exit(-1);
  }

  const char *test_directory = argv[1];
  test_openat(test_directory);
  test_fstatat(test_directory);

  puts("PASSED");
  return 0;
}
