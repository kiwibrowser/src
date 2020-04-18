/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>
#include <unistd.h>

#include "native_client/src/untrusted/irt/irt_pnacl_translator_common.h"

/*
 * If |env_str| is an environment list entry whose variable name starts
 * with |prefix|, this returns the environment variable's value.
 * Otherwise, this returns NULL.
 */
char *env_var_prefix_match(char *env_str, const char *prefix) {
  size_t prefix_len = strlen(prefix);
  if (strncmp(env_str, prefix, prefix_len) == 0) {
    char *match = strchr(env_str + prefix_len, '=');
    if (match != NULL) {
      return match + 1;
    }
  }
  return NULL;
}

/*
 * Returns the number of environment variables that will be matched by
 * env_var_prefix_match().  i.e. Returns the number of environment
 * variables whose names start with |prefix|.
 */
size_t env_var_prefix_match_count(const char *prefix) {
  size_t count = 0;
  char **env;
  for (env = environ; *env != NULL; ++env) {
    if (env_var_prefix_match(*env, prefix) != NULL)
      ++count;
  }
  return count;
}
