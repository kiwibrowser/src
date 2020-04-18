/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Simple environment cleanser to remove all but a whitelisted set of
 * environment variables deemed safe/appropriate to export to NaCl
 * modules.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ENV_CLEANSER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ENV_CLEANSER_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClEnvCleanser {
  /* private */
  int with_whitelist;
  int with_passthrough;
  char const **cleansed_environ;
};

void NaClEnvCleanserCtor(struct NaClEnvCleanser *self,
                         int with_whitelist, int with_passthrough);

/*
 * Initializes the NaClEnvCleanser.  Filters the environment at envp
 * and saves the result in the object.  The lifetime of the string
 * table and associated strings at envp must be at least that of the
 * NaClEnvCleanser object, and should not change between the call to
 * NaClEnvCleanserInit and to NaClEnvCleanserDtor.
 */
int NaClEnvCleanserInit(struct NaClEnvCleanser *self, char const *const *envp,
    char const *const *extra_env);

/*
 * Returns the filtered environment.
 */
char const *const *NaClEnvCleanserEnvironment(struct NaClEnvCleanser *self);

/*
 * Frees memory associated with the NaClEnvCleanser object.
 */
void NaClEnvCleanserDtor(struct NaClEnvCleanser *self);

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ENV_CLEANSER_H_
