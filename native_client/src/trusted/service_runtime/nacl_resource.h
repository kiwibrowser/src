/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_RESOURCE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_RESOURCE_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Pseudo device name for NACL_EXE_STD{OUT,ERR}.
 */
#define NACL_RESOURCE_DEBUG_WARNING   "DEBUG_ONLY:"
#define NACL_RESOURCE_FILE_PREFIX     "file:"
#define NACL_RESOURCE_FILE_DEV_NULL   "/dev/null"

struct NaClResource;

struct NaClResourceSchemes {
  char const      *scheme_prefix;
  int             default_scheme;
  /*
   * |default_scheme| is a bool.  If no scheme prefixes match, try
   * Open with this.  There should be only one default scheme per
   * scheme_table.
   */

  /*
   * The reason to separate out these functions is to make resource
   * namespace separation clearer.  Files, which require --no-sandbox
   * to disable the outer sandbox, allow arbitrary paths for logging
   * untrusted code output; pseudo-devices (for postmessage) is
   * (currently) a namespace of one entry.
   *
   * |nacl_flags| should be NACL_ABI_ versions of |flags| and should
   * be consistent.  This is typically determined at compile time, but
   * the utility NaClHostDescMapOpenFlags can be used to convert
   * nacl_flags values to flags values.
   *
   * |mode| should be file access mode (if file, if O_CREAT, if appropriate).
   */
  struct NaClDesc *(*Open)(struct NaClResource  *resource,
                           char const           *resource_specifier_rest,
                           int                  nacl_flags,
                           int                  mode /* 0777 etc */,
                           int                  allow_debug /* bool */);
};


struct NaClResource {
  /*
   * no vtbl with virtual dtor, since (for now) only object creator
   * should dtor/delete, and there are no other virtual functions
   * needed.
   */
  struct NaClResourceSchemes const  *schemes;
  size_t                            num_schemes;
};

/*
 * NaClResourceOpen handles NACL_RESOURCE_DEBUG_WARNING_PREFIX checks
 * (and stripping), NACL_RESOURCE_{FILE,DEV}_PREFIX dispatch.
 *
 * This function does not take a descriptor number to directly modify
 * the descriptor array and require the caller to invoke
 * NaClAppSetDesc(), since the API allows other uses of the returned
 * NaClDesc object than just for redirection.
 */
struct NaClDesc *NaClResourceOpen(struct NaClResource *self,
                                  char const          *resource_locator,
                                  int                 nacl_flags,
                                  int                 mode);

/*
 * Subclasses can expand on the NaClResource base class, e.g., add
 * startup phase information so that the Open functions can get the
 * NaClApp pointer, etc.  The sole base class member function,
 * NaClResourceOpen, is unaware of startup phases and relies on the
 * scheme table's Open function to do the right thing.
 */
struct NaClResourceNaClApp {
  struct NaClResource base;
  struct NaClApp      *nap;
};

int NaClResourceNaClAppCtor(struct NaClResourceNaClApp        *self,
                            struct NaClResourceSchemes const  *scheme_tbl,
                            size_t                            num_schemes,
                            struct NaClApp                    *nap);

/*
 * Invoke Ctor with standard resource schemes.
 */
int NaClResourceNaClAppInit(struct NaClResourceNaClApp        *self,
                            struct NaClApp                    *nap);

EXTERN_C_END

#endif
