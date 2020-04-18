/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>

int main(void) {
  pthread_exit(NULL);
  /* Should never get here */
  return 1;
}
