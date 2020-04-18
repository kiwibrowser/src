// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LOGGING_H_
#define LOGGING_H_

#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK_ARGS(COND, ERR)                                          \
  "FAILED CHECK(%s) @ %s:%d (errno: %s)\n", #COND, __FILE__, __LINE__, \
      strerror(ERR)

#define CHECK(x)                                              \
  do {                                                        \
    if (!(x)) {                                               \
      const int e = errno;                                    \
      __android_log_print(ANDROID_LOG_FATAL, "atrace_helper", \
                          CHECK_ARGS(x, e));                  \
      fprintf(stderr, "\n" CHECK_ARGS(x, e));                 \
      fflush(stderr);                                         \
      abort();                                                \
    }                                                         \
  } while (0)

#endif  // LOGGING_H_
