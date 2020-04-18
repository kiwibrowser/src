/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <wchar.h>

int main(void) {
  wchar_t *wcs;
  wchar_t *stopwcs;
  long long val;

  wcs = L"1234567890123456789";
  printf("wcs = `%ls`\n", wcs);
  val = wcstoll(wcs, &stopwcs, 10);
  printf("wcstoll = %lld\n", val);
  printf("Stopped scan at `%ls`\n", stopwcs);
  if (val != 1234567890123456789LL) {
    printf("Wrong answer\n");
    return 1;
  }
  printf("OK\n");
  return 0;
}
