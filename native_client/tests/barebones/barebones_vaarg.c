/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib
 */

#include "barebones.h"
#include <stdarg.h>

/* globals are intentional to confuse optimizer */
int errors = 55;
int count = 0;
char buffer[16];

int cmp(long l) {
  ++count;

  if (l != count) {
    myprint("Error is ");
    myhextochar(l, buffer);
    myprint(buffer);
    myprint(" expected ");
    myhextochar(count, buffer);
    myprint(buffer);
    myprint("\n");

    return 1;
  }

  return 0;
}


int dump(const char *fmt, ...) {
  int errors = 0;
  int i;

  va_list ap;
  va_start(ap, fmt);

  myprint(fmt);
  myprint("\n");
  count = 0;

  for (i = 0; fmt[i]; ++i) {
    if (fmt[i] == 'L') {
      long l = va_arg(ap, long);
      errors += cmp(l);
    } else if (fmt[i] == 'Q') {
      long long ll = (long long) va_arg(ap, long long);
      errors += cmp((long)((ll >> 32) & 0xffffffffL));
      errors += cmp((long)(ll & 0xffffffffL));
    } else {
      myprint("Error bad format");
      errors += 100;
    }
  }
  va_end(ap);

  return errors;
}


void test(void) {
  errors += dump("QQQL",
                 0x0000000100000002LL,
                 0x0000000300000004LL,
                 0x0000000500000006LL,
                 0x00000007L);

  errors += dump("QQLQ",
                 0x0000000100000002LL,
                 0x0000000300000004LL,
                 0x00000005L,
                 0x0000000600000007LL);

  errors += dump("QLQQ",
                 0x0000000100000002LL,
                 0x00000003L,
                 0x0000000400000005LL,
                 0x0000000600000007LL);

  errors += dump("LQQQ",
                 0x00000001L,
                 0x0000000200000003LL,
                 0x0000000400000005LL,
                 0x0000000600000007LL);

  errors += dump("QQQ",
                 0x0000000100000002LL,
                 0x0000000300000004LL,
                 0x0000000500000006LL);

  errors += dump("LLLLLL",
                 0x00000001L,
                 0x00000002L,
                 0x00000003L,
                 0x00000004L,
                 0x00000005L,
                 0x00000006L);
}


int main(void) {
  test();
  NACL_SYSCALL(exit)(errors);
  /* UNREACHABLE */
  return 0;
}
