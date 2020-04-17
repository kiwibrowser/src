// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace openscreen {
namespace platform {

extern int g_log_fd;

void LogInit(const char* filename) {
  struct stat st = {};
  if (stat(filename, &st) == -1 && errno == ENOENT) {
    if (mkfifo(filename, 0644) == 0) {
      g_log_fd = open(filename, O_WRONLY);
    } else {
      g_log_fd = STDOUT_FILENO;
    }
  } else if (S_ISFIFO(st.st_mode)) {
    g_log_fd = open(filename, O_WRONLY);
  } else {
    g_log_fd = STDOUT_FILENO;
  }
}

}  // namespace platform
}  // namespace openscreen
