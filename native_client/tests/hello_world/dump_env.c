/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdio.h>

int main(int argc, char *argv[], char *envp[]) {
  int i;
  for (i = 0 ; i < argc; i++) {
    printf("argv[%d] = [%s]\n", i, argv[i]);
  }

  if (envp) {
    for (i = 0 ; envp[i] != 0; i++) {
      printf("evp[%d] = [%s]\n", i, envp[i]);
    }
  }

  printf("# argc %d argv %p\n", argc, (void *) argv);
  printf("# nenv %d envp %p\n", i, (void *) envp);
  return 0;
}
