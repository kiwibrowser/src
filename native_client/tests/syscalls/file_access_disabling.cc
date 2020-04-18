/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "native_client/src/include/nacl_assert.h"


// This test checks that filesystem access is disabled by default, when "-a"
// is not passed to sel_ldr.
//
// To ensure that the checks do not pass by accident, we check the behaviour
// of the filesystem syscalls with and without "-a".
//
// This is intended to cover all of the filename-based syscalls defined in
// sys_filename.c -- with the exception of utimes(), which is not implemented
// (other than to return ENOSYS).

static bool g_enabled;

static void check_result(const char *msg, bool passed) {
  if (g_enabled) {
    ASSERT_MSG(passed, msg);
  } else {
    ASSERT_MSG(!passed, msg);
    // All of the filename-based syscalls return EACCES when filesystem
    // access is disabled.  However, IRT interface wrappers return ENOSYS
    // when the IRT interface is not available.
    if (errno != EACCES && errno != ENOSYS) {
      printf("Got unexpected errno: %d\n", errno);
      exit(1);
    }
    // Reset in case the expected errno leaks through to the next check.
    errno = 0;
  }
}

int main(int argc, char **argv) {
  // Read command line arguments.
  ASSERT_EQ(argc, 3);
  if (strcmp(argv[1], "--enabled") == 0) {
    g_enabled = true;
  } else if (strcmp(argv[1], "--disabled") == 0) {
    g_enabled = false;
  } else {
    fprintf(stderr, "Unrecognised argument: %s\n", argv[1]);
    return 1;
  }
  const char *temp_dir_path = argv[2];

  // Change directory first, so that the files we create go into a temporary
  // directory.
  check_result("chdir", chdir(temp_dir_path) == 0);

  int fd = open("test_file", O_WRONLY | O_CREAT, 0666);
  check_result("open", fd >= 0);
  if (fd >= 0) {
    int rc = close(fd);
    ASSERT_EQ(rc, 0);
  }

  fd = open("test_file", O_RDONLY);
  check_result("open", fd >= 0);
  if (fd >= 0) {
    int rc = close(fd);
    ASSERT_EQ(rc, 0);
  }

  struct stat st;
  check_result("stat", stat("test_file", &st) == 0);
  check_result("lstat", lstat("test_file", &st) == 0);
  check_result("access", access("test_file", F_OK) == 0);
  check_result("truncate", truncate("test_file", 100) == 0);
  check_result("chmod", chmod("test_file", 0666) == 0);

  check_result("link", link("test_file", "test_file2") == 0);
  check_result("rename", rename("test_file2", "test_file") == 0);

  check_result("unlink", unlink("test_file") == 0);

  check_result("symlink", symlink("symlink_dest", "my_symlink") == 0);
  char buf[100];
  int length = readlink("my_symlink", buf, sizeof(buf));
  check_result("readlink", length >= 0);
  check_result("unlink", unlink("my_symlink") == 0);

  check_result("mkdir", mkdir("subdir", 0777) == 0);
  check_result("rmdir", rmdir("subdir") == 0);

  char *cwd = getcwd(NULL, 0);
  check_result("getcwd", cwd != NULL);
  if (cwd != NULL)
    free(cwd);

  return 0;
}
