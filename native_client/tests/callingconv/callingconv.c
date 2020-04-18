/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#undef NDEBUG
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <callingconv.h>

/* Note, not all of these are necessarily used */
t_charp v_t_charp[16];
t_int v_t_int[16];
t_long v_t_long[16];
t_llong v_t_llong[16];
t_double v_t_double[16];
t_ldouble v_t_ldouble[16];
t_char v_t_char[16];
t_short v_t_short[16];
t_float v_t_float[16];
t_tiny v_t_tiny[16];
t_big v_t_big[16];

/* Keep track of current function, call, and satisfied asserts */
int current_module = -1;
int current_call = -1;
int current_function = -1;
int *current_index_p = NULL;
int assert_count = 0;

extern const char *script_argv;
void module0(void) __attribute__((weak));
void module1(void) __attribute__((weak));
void module2(void) __attribute__((weak));
void module3(void) __attribute__((weak));
void module4(void) __attribute__((weak));
void module5(void) __attribute__((weak));

int main(int argc, const char *argv[]) {
  if (module0) module0();
  if (module1) module1();
  if (module2) module2();
  if (module3) module3();
  if (module4) module4();
  if (module5) module5();

  printf("generate.py arguments: %s\n", script_argv);
  printf("SUCCESS: %d calls OK.\n", assert_count);
  return 0;
}


/* Helper for setting values in tiny_t struct */
void set_tiny(t_tiny *ptr, char a, short b) {
  ptr->a = a;
  ptr->b = b;
}

/* Helper for setting values in big_t struct */
void set_big(t_big *ptr, char a, char b, int c, char d, int e, char f,
             long long g, char h, int i, char j, short k, char l,
             double m, char n) {
  ptr->a = a;
  ptr->b = b;
  ptr->c = c;
  ptr->d = d;
  ptr->e = e;
  ptr->f = f;
  ptr->g = g;
  ptr->h = h;
  ptr->i = i;
  ptr->j = j;
  ptr->k = k;
  ptr->l = l;
  ptr->m = m;
  ptr->n = n;
}


void assert_func(int condition, const char *expr, const char *file, int line) {
  assert_count++;
  if (!condition) {
    printf("Assertion Failure: %s\n", expr);
    printf("Location         : %s:%d\n", file, line);
    printf("Module           : %d\n", current_module);
    printf("Call Name        : C%d\n", current_call);
    printf("Function Name    : F%d\n", current_function);
    if (current_index_p != NULL)
      printf("Argument         : a_%d\n", *current_index_p);
    exit(1);
  }
}

#define EQ(_v)  (x._v == y._v)

int tiny_cmp(const t_tiny x, const t_tiny y) {
  return EQ(a) && EQ(b);
}

int big_cmp(const t_big x, const t_big y) {
  return EQ(a) && EQ(b) && EQ(c) && EQ(d) &&
         EQ(e) && EQ(f) && EQ(g) && EQ(h) &&
         EQ(i) && EQ(j) && EQ(k) && EQ(l) && EQ(m) && EQ(n);
}
