/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * getcwd() implementation based on the lower level
 * __getcwd_without_malloc().
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/getcwd.h"

char *getcwd(char *buffer, size_t len) {
  int allocated = 0;
  int do_realloc = 0;

  /* If buffer is NULL, allocate a buffer. */
  if (buffer == NULL) {
    if (len == 0) {
      len = PATH_MAX;
      do_realloc = 1;
    }

    buffer = (char *) malloc(len);
    if (buffer == NULL) {
      errno = ENOMEM;
      return NULL;
    }
    allocated = 1;
  } else if (len == 0) {
    /* Non-NULL buffer and zero size results in EINVAL */
    errno = EINVAL;
    return NULL;
  }

  char *rtn = __getcwd_without_malloc(buffer, len);
  if (allocated) {
    if (rtn == NULL) {
      free(buffer);
    } else if (do_realloc) {
      rtn = (char *) realloc(rtn, strlen(rtn) + 1);
    }
  }

  return rtn;
}
