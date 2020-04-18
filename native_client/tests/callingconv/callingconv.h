/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TESTS_CALLING_CONV_H
#define __TESTS_CALLING_CONV_H

typedef struct {
  char a;
  short b;
} t_tiny;

typedef struct {
  char a;
  char b;
  int c;
  char d;
  int e;
  char f;
  long long g;
  char h;
  int i;
  char j;
  short k;
  char l;
  double m;
  char n;
} t_big;

/* Comparison functions */
#define TINY_CMP(_x, _y)   (tiny_cmp((_x), (_y)))
#define BIG_CMP(_x, _y)    (big_cmp((_x), (_y)))
int tiny_cmp(const t_tiny x, const t_tiny y);
int big_cmp(const t_big x, const t_big y);


/* Setters */
#define SET_TINY(obj, a, b)  \
    (set_tiny(&(obj), a, b))

#define SET_BIG(obj, a, b, c, d, e, f, g, h, i, j, k, l, m, n) \
    (set_big(&(obj), a, b, c, d, e, f, g, h, i, j, k, l, m, n))

void set_tiny(t_tiny *ptr, char a, short b);

void set_big(t_big *ptr, char a, char b, int c, char d, int e, char f,
             long long g, char h, int i, char j, short k, char l,
             double m, char n);

/* types used by the modules  */
typedef char*        t_charp;
typedef int          t_int;
typedef long         t_long;
typedef long long    t_llong;
typedef double       t_double;
typedef long double  t_ldouble;
typedef char         t_char;
typedef short        t_short;
typedef float        t_float;

/*
 * check variables. These are global arrays
 * which contain copies of the arguments passed to the function.
 * The test functions compare their arguments with the check
 * variables to ensure a match.
 */
extern t_charp v_t_charp[16];
extern t_int v_t_int[16];
extern t_long v_t_long[16];
extern t_llong v_t_llong[16];
extern t_double v_t_double[16];
extern t_ldouble v_t_ldouble[16];
extern t_char v_t_char[16];
extern t_short v_t_short[16];
extern t_float v_t_float[16];
extern t_tiny v_t_tiny[16];
extern t_big v_t_big[16];

/* Used by the modules to keep track of the current location */
extern int current_module;
extern int current_call;
extern int current_function;
extern int *current_index_p;
extern int assert_count;

#define SET_CURRENT_MODULE(id)   (current_module = (id))
#define SET_CURRENT_FUNCTION(id) (current_function = (id))
#define SET_CURRENT_CALL(id)     (current_call = (id))
#define SET_INDEX_VARIABLE(id)   (current_index_p = &(id))

/* Used by the modules to compare arguments to the expected value */
#define ASSERT(cond)          (assert_func((cond), #cond, __FILE__, __LINE__))

void assert_func(int condition, const char *expr, const char *file, int line);

#endif
