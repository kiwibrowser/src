/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

#define ASSERT(cond, message)                \
  if (!(cond)) {                             \
    fputs(#cond ": ", stderr);               \
    fputs(message, stderr);                  \
    fputs("\n\n", stderr);                   \
    abort();                                 \
  }

/* C++ does not like us casting function pointers to data pointers */
#define FUNPTR2PTR(a)   ((void*)((long)a))

#define ARRAY_SIZE_UNSAFE(arr) ((sizeof arr)/sizeof arr[0])
