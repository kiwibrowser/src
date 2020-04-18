/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <string.h>

#include "native_client/src/shared/gio/gio.h"

int main(void) {
  char buf[8 * 1024];
  size_t ix;
  struct GioFile gf;

  for (ix = 0; ix < sizeof buf - 1; ++ix) {
    buf[ix] = "0123456789abcdef"[ix & 0xf];
  }
  buf[sizeof buf - 1] = '\0';
  if (!GioFileRefCtor(&gf, stdout)) {
    fprintf(stderr, "GioFileRefCtor failed\n");
    return 1;
  }
  gprintf((struct Gio *) &gf, "data: %s\n", buf);
  return 0;
}
