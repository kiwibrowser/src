/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This is an example library, for testing the ConvertToPSO pass. */

/* Test thread-local variables (TLS) */

extern int module_a_var;

/*
 * Test zero-initialized (BSS) variables.  Test that they are placed at the
 * end of the TLS template even if they appear first in the IR or source.
 */
__thread int tls_bss_var1;
__thread int tls_bss_var_aligned __attribute__((aligned(256)));

__thread int tls_var1 = 123;
__thread int *tls_var2 = &module_a_var;
__thread int tls_var_aligned __attribute__((aligned(256))) = 345;

extern __thread int tls_var_exported1;
extern __thread int tls_var_exported2;

int *get_tls_bss_var1(void) {
  return &tls_bss_var1;
}

int *get_tls_bss_var_aligned(void) {
  return &tls_bss_var_aligned;
}

int *get_tls_var1(void) {
  return &tls_var1;
}

/* Test handling of ConstantExprs. */
int *get_tls_var1_addend(void) {
  return &tls_var1 + 1;
}

int **get_tls_var2(void) {
  return &tls_var2;
}

int *get_tls_var_aligned(void) {
  return &tls_var_aligned;
}

int *get_tls_var_exported1(void) {
  return &tls_var_exported1;
}

int *get_tls_var_exported2(void) {
  return &tls_var_exported2;
}
