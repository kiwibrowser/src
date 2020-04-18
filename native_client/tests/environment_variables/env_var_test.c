/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * In the glibc dynamic linking case, CommandSelLdrTestNacl
 * passes LD_LIBRARY_PATH in the environment for tests.
 * Ignore this variable for purposes of this test.
 */
static bool skip_env_var(const char *envstring) {
#ifdef __GLIBC__
  static const char kLdLibraryPath[] = "LD_LIBRARY_PATH=";
  if (!strncmp(envstring, kLdLibraryPath, sizeof(kLdLibraryPath) - 1))
    return true;
#endif
  return false;
}

int main(int argc, char **argv, char **env) {
  int count = 0;
  char **ptr;

  for (ptr = environ; *ptr != NULL; ptr++) {
    if (!skip_env_var(*ptr))
      count++;
  }
  printf("%i environment variables\n", count);
  for (ptr = environ; *ptr != NULL; ptr++) {
    if (!skip_env_var(*ptr))
      puts(*ptr);
  }

  assert(env == environ);

  return 0;
}
