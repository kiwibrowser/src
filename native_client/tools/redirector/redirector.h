/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TOOLS_REDIRECTOR_H_
#define NATIVE_CLIENT_TOOLS_REDIRECTOR_H_

#include <wchar.h>

typedef struct {
  const wchar_t *from;
  const wchar_t *to;
  const wchar_t *args;
} redirect_t;

#ifndef REDIRECT_DATA
#define REDIRECT_DATA "redirector_table.txt"
#endif

const redirect_t redirects[] = {
#include REDIRECT_DATA
};

#endif
