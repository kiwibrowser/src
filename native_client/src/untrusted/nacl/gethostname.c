/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

int gethostname(char *name, size_t len) {
  static const char *hostname = "naclhost";
  if (len <= strlen(hostname)) {
    errno = ENAMETOOLONG;
    return -1;
  }

  strcpy(name, hostname);
  return 0;
}
