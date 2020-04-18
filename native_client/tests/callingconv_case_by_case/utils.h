/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_CALLINGCONV_SMALL_UTILS_H
#define NATIVE_CLIENT_TESTS_CALLINGCONV_SMALL_UTILS_H

#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* We sometimes reuse the same source file, so __FILE__ doesn't make sense
 * for debugging. Set up another identifier for which "file" you are in.
 */
#if defined(MODULE0)
#define __MODULE__ ("MODULE0:" __FILE__)
#elif defined(MODULE1)
#define __MODULE__ ("MODULE1:" __FILE__)
#elif defined(MODULE2)
#define __MODULE__ ("MODULE2:" __FILE__)
#elif defined(MODULE3)
#define __MODULE__ ("MODULE3:" __FILE__)
#else
#error "Must define MODULE0. or MODULE1, 2 or 3 in preprocessor!"
#endif  /* defined(MODULE0) */

/* NOTE: this also depends on having a __CUR_FUNC__ variable. */
#define ASSERT(cond, message)                                           \
  if (!(cond)) {                                                        \
    fprintf(stderr, #cond ": " message " file: %s, func: %s, "          \
            "line: %i\n\n",  __MODULE__, __func__, __LINE__);           \
    abort();                                                            \
  }

#define ASSERT_EQ(lhs, rhs, message)                                    \
  if ((lhs) != (rhs)) {                                                 \
  fprintf(stderr, #lhs " != " #rhs ": "                                 \
          message " file: %s, func: %s, "                               \
          "line: %i\n\n",  __MODULE__, __func__, __LINE__);             \
    abort();                                                            \
  }

/* Some golden constants to check and see if they were preserved across
 * function call boundaries.
 */

#define KCHAR1 ((char) (1 << 7 | 1 << 5 | 1 << 3 | 1))
#define KCHAR2 (KCHAR1 + 1)
#define KCHAR3 (KCHAR1 + 3)
#define KCHAR4 (KCHAR1 + 7)
#define KI161 ((int16_t) (1 << 15 | 1 << 13 | 1 << 11 | KCHAR1))
#define KI162 (KI161 + 1)
#define KI163 (KI161 + 3)
#define KI164 (KI161 + 7)
#define KI321 ((int32_t) (1 << 31 | 1 << 29 |  1 << 27 | 1 << 25 | KI161))
#define KI322 (KI321 + 1)
#define KI323 (KI321 + 3)
#define KI324 (KI321 + 7)
#define KFLOAT1 FLT_MAX
#define KFLOAT2 FLT_MIN
#define KFLOAT3 (-FLT_MAX)
#define KFLOAT4 (-FLT_MIN)
#define KDOUBLE1 DBL_MAX
#define KDOUBLE2 DBL_MIN
#define KDOUBLE3 (-DBL_MAX)
#define KDOUBLE4 (-DBL_MIN)
#define KI641 ((int64_t) (1LL << 63 | 1LL << 61 | 1LL << 59 | 1LL << 57 | \
                          1LL << 55 | 1LL << 53 | 1LL << 47 | 1LL << 45 | \
                          KI321))
#define KI642 (KI641 + 1)
#define KI643 (KI641 + 3)
#define KI644 (KI641 + 7)

#define KPTR1 ((void *)(KI321))
#define KPTR2 ((void *)(KI322))

/* If there was a way to test _Bool at the same time, that would be good. */
#define KBOOL1 ((bool) false)
#define KBOOL2 ((bool) true)

#endif  /* NATIVE_CLIENT_TESTS_CALLINGCONV_SMALL_UTILS_H */
