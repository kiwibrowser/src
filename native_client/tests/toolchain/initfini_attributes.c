/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>


__attribute__((constructor))
static void init_func(void) {
  printf("init_func with default priority\n");
}

__attribute__((constructor(200)))
static void init_func200(void) {
  printf("init_func with priority 200\n");
}

__attribute__((constructor(300)))
static void init_func300(void) {
  printf("init_func with priority 300\n");
}

__attribute__((destructor))
static void fini_func(void) {
  printf("fini_func with default priority\n");
}

__attribute__((destructor(200)))
static void fini_func200(void) {
  printf("fini_func with priority 200\n");
}

__attribute__((destructor(300)))
static void fini_func300(void) {
  printf("fini_func with priority 300\n");
}

int main(void) {
  printf("in main()\n");
  return 0;
}
