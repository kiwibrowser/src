/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/trusted/service_runtime/env_cleanser.h"
#include "native_client/src/trusted/service_runtime/env_cleanser_test.h"

/*
 * Everything that starts with this prefix is allowed (but the prefix is
 * stripped away).
*/
#define NACL_ENV_PREFIX "NACLENV_"
#define NACL_ENV_PREFIX_LENGTH 8

void NaClEnvCleanserCtor(struct NaClEnvCleanser *self,
                         int with_whitelist, int with_passthrough) {
  self->with_whitelist = with_whitelist;
  self->with_passthrough = with_passthrough;
  self->cleansed_environ = (char const **) NULL;
}

/*
 * Environment variables names are from IEEE Std 1003.1-2001, with
 * additional ones from locale(7) from glibc / linux.  The entries
 * must be sorted, in ASCII order, for the bsearch to run correctly.
 */
/* static -- not static for testing */
char const *const kNaClEnvWhitelist[] = {
  "LANG",
  "LC_ADDRESS",
  "LC_ALL",
  "LC_COLLATE",
  "LC_CTYPE",
  "LC_IDENTIFICATION",
  "LC_MEASUREMENT",
  "LC_MESSAGES",
  "LC_MONETARY",
  "LC_NAME",
  "LC_NUMERIC",
  "LC_PAPER",
  "LC_TELEPHONE",
  "LC_TIME",
  "NACLVERBOSITY",
  "NACL_PLUGIN_DEBUG",      /* Chromium's plugin/utility.cc */
  NULL,
};

/* left arg is key, right arg is table entry */
static int EnvCmp(void const *vleft, void const *vright) {
  char const *left = *(char const *const *) vleft;
  char const *right = *(char const *const *) vright;
  char cleft, cright;

  while ((cleft = *left) == (cright = *right)
         && '\0' != cleft
         && '\0' != cright) {
    ++left;
    ++right;
  }
  if ('=' == cleft && '\0' == cright) {
    return 0;
  }
  return (0xff & cleft) - (0xff & cright);
}

int NaClEnvIsPassThroughVar(char const *env_entry) {
  return strlen(env_entry) > NACL_ENV_PREFIX_LENGTH &&
      0 == strncmp(env_entry, NACL_ENV_PREFIX,
          NACL_ENV_PREFIX_LENGTH);
}

int NaClEnvInWhitelist(char const *env_entry) {
  return NULL != bsearch((void const *) &env_entry,
                         (void const *) kNaClEnvWhitelist,
                         NACL_ARRAY_SIZE(kNaClEnvWhitelist) - 1,  /* NULL */
                         sizeof kNaClEnvWhitelist[0],
                         EnvCmp);
}

static int VarIsInExtraEnv(char const *env_entry,
                           char const *const *extra_env) {
  if (extra_env != NULL) {
    const char *name_end = strchr(env_entry, '=');
    if (name_end != NULL) {
      size_t compare_len = name_end + 1 - env_entry;
      char const *const *ep;
      for (ep = extra_env; *ep != NULL; ++ep) {
        if (strncmp(env_entry, *ep, compare_len) == 0)
          return 1;
      }
    }
  }
  return 0;
}

/* PRE: sizeof(char *) is a power of 2 */

/*
 * Initializes the object with a filtered environment.
 *
 * May return false on errors, e.g., out-of-memory.
 */
int NaClEnvCleanserInit(struct NaClEnvCleanser *self, char const *const *envp,
    char const * const *extra_env) {
  char const *const *p;
  size_t num_env = 0;
  size_t ptr_bytes = 0;
  const size_t kMaxSize = ~(size_t) 0;
  const size_t ptr_size_mult_overflow_mask = ~(kMaxSize / sizeof *envp);
  char const **ptr_tbl;
  size_t env;

  /*
   * let n be a size_t.  if n & ptr_size_mult_overflow_mask is set,
   * then n*sizeof(void *) will have an arithmetic overflow.
   */

  if ((NULL == envp || NULL == *envp) &&
      (NULL == extra_env || NULL == *extra_env)) {
    self->cleansed_environ = NULL;
    return 1;
  }

  /*
   * The explicit extra variables go before the filtered inherited ones.
   */
  if (extra_env) {
    for (p = extra_env; NULL != *p; ++p) {
      if (num_env == kMaxSize) {
        /* would overflow */
        return 0;
      }
      ++num_env;
    }
  }

  if (envp) {
    for (p = envp; NULL != *p; ++p) {
      if (VarIsInExtraEnv(*p, extra_env) ||
          (!self->with_passthrough &&
           !(self->with_whitelist && NaClEnvInWhitelist(*p)) &&
           !NaClEnvIsPassThroughVar(*p))) {
        continue;
      }
      if (num_env == kMaxSize) {
        /* would overflow */
        return 0;
      }
      ++num_env;
    }
  }

  /* pointer table -- NULL pointer terminated */
  if (0 != ((1 + num_env) & ptr_size_mult_overflow_mask)) {
    return 0;
  }
  ptr_bytes = (1 + num_env) * sizeof(*envp);

  ptr_tbl = (char const **) malloc(ptr_bytes);
  if (NULL == ptr_tbl) {
    return 0;
  }

  env = 0;
  if (extra_env) {
    for (p = extra_env; NULL != *p; ++p) {
      ptr_tbl[env] = *p;
      ++env;
    }
  }

  if (envp) {
    /* this assumes no other thread is tweaking envp */
    for (p = envp; NULL != *p; ++p) {
      if (VarIsInExtraEnv(*p, extra_env)) {
        continue;
      } else if (NaClEnvIsPassThroughVar(*p)) {
        ptr_tbl[env] = *p + NACL_ENV_PREFIX_LENGTH;
      } else if (self->with_passthrough ||
                 (self->with_whitelist && NaClEnvInWhitelist(*p))) {
        ptr_tbl[env] = *p;
      } else {
        continue;
      }
      ++env;
    }
  }

  if (num_env != env) {
    free((void *) ptr_tbl);
    return 0;
  }

  ptr_tbl[env] = NULL;
  self->cleansed_environ = ptr_tbl;

  return 1;
}

char const *const *NaClEnvCleanserEnvironment(struct NaClEnvCleanser *self) {
  return (char const *const *) self->cleansed_environ;
}

void NaClEnvCleanserDtor(struct NaClEnvCleanser *self) {
  free((void *) self->cleansed_environ);
  self->cleansed_environ = NULL;
}
