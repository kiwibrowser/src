/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test basic floating point operations
 */

#include <stdio.h>
#include <stdlib.h>

#if 1
#define PRINT_FLOAT(mesg, prec, val)\
 printf(mesg ": %." #prec  "f\n", (double) (val))
#else
#define PRINT_FLOAT(mesg, prec, val)\
  printf(mesg ": %d\n", (int) (val))
#endif

#define PRINT_INT(mesg, val)\
  printf(mesg ": %d\n", (int) (val))


/* TODO(robertm): maybe use macro */
int main_double(int argc, char* argv[]) {
  int i;
  double last = 0.0;
  double x;
  printf("double\n");

  for (i = 1; i < argc; ++i) {
    printf("val str: %s\n", argv[i]);

    x = strtod(argv[i], 0);
    PRINT_INT("val int", x);
    PRINT_FLOAT("val flt",9, x);
    PRINT_FLOAT("last", 9, last);
    PRINT_FLOAT("+", 9, last + x);
    PRINT_FLOAT("-", 9, last - x);
    PRINT_FLOAT("*", 9, last * x);
    PRINT_FLOAT("/", 9, last / x);
    printf("\n");
    last = x;
  }
  return 0;
}


/* TODO(robertm): maybe use macro */
int main_float(int argc, char* argv[]) {
  int i;
  float last = 0.0;
  float x;
  printf("float\n");

  for (i = 1; i < argc; ++i) {
    printf("val str: %s\n", argv[i]);

    x = strtof(argv[i], 0);
    PRINT_INT("val int", x);
    PRINT_FLOAT("val flt",9, x);
    PRINT_FLOAT("last", 3, last);
    PRINT_FLOAT("+", 3, last + x);
    PRINT_FLOAT("-", 3, last - x);
    PRINT_FLOAT("*", 3, last * x);
    PRINT_FLOAT("/", 3, last / x);
    printf("\n");
    last = x;
  }
  return 0;
}


int main(int argc, char* argv[]) {
  main_float(argc, argv);
  main_double(argc, argv);
  return 0;
}
