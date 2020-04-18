/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

extern void sym_a(void);
extern void sym_b(void);

int main() {
  sym_a();
  sym_b();
  return 0;
}
