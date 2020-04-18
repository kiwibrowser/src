/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * These are the only functions that may be called from
 * ppapi_callingconv_test.cpp.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "support.h"

union AllValues g_input_values;

void emit_integer(const char* m, int64_t val) {
  printf("%s: %lld\n", m, val);
}

void emit_string(const char* s) {
  puts(s);
}

void emit_pointer(const char* m, const void* val) {
  printf("%s: %p\n", m, val);
}

void randomize(void) {
  char* data = (char*) &g_input_values;
  for (size_t i = 0; i < sizeof(g_input_values); ++i) {
    data[i] = (char) lrand48();
  }
}

void initialize(int argc, char* argv[]) {
  int x = 0;
  if (argc >= 2) {
    char* cp = argv[1];
    int c;
    while ((c = *cp++)) {
      x  = 10 * x + (c - '0');
    }
  }
  srand48(x);
  randomize();
}

