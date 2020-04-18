/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* make sure these variables get placed in tls_data */
__thread int tls_initialized_var1 = 67;
__thread int tls_initialized_var2 = 1;
__thread int tls_initialized_var3 = 1;

int main(void) {
  return tls_initialized_var1 + tls_initialized_var2 + tls_initialized_var3;
}
