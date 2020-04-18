/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef UTIL_H
#define UTIL_H

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))
#define PUBLIC __attribute__((visibility("default")))
#define ALIGN(A, B) (((A) + (B)-1) / (B) * (B))
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

#endif
