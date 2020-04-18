/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <utime.h>

/*
 * TODO(sbc): remove this once utimes declaration gets added to the newlib
 * headers.
 */
int utimes(const char *filename, const struct timeval tv[2]);

/*
 * Implementation of utime() based on utimes().  utime() works just like
 * utimes() but only supports timestamps with a granularity of one second
 * (time_t). This means we we can use utimes() to implement utime() by simply
 * setting tv_usec fields to zero.
 */
int utime(const char *filename, const struct utimbuf *buf) {
  struct timeval times[2];
  struct timeval *tv = NULL;
  if (buf != NULL) {
    times[0].tv_sec = buf->actime;
    times[1].tv_sec = buf->modtime;
    times[0].tv_usec = 0;
    times[1].tv_usec = 0;
    tv = times;
  }

  return utimes(filename, tv);
}
