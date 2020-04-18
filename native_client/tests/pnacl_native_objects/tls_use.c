/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

extern __thread int tlsvar;
extern int check_set_tls(int expected, int value);

int main() {
  tlsvar = 42;
  int errors = 0;
  int oldval = check_set_tls(42, -123);
  if (oldval != 42)
    errors++;
  if (tlsvar != -123)
    errors++;
  return errors;
}
