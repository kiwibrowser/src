/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/trusted/service_runtime/nacl_resource.h"

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nacl_desc_null.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

struct NaClDesc *NaClResourceOpen(struct NaClResource *self,
                                  char const          *resource_locator,
                                  int                 nacl_flags,
                                  int                 mode) {
  size_t  ix;
  size_t  default_ix = ~(size_t) 0;  /* greater than self->num_schemes */
  size_t  prefix_len;
  int     allow_debug = 0;

  NaClLog(4, "NaClResourceOpen(*,\"%s\",0x%x,0%o)\n",
          resource_locator, nacl_flags, mode);
  prefix_len = strlen(NACL_RESOURCE_DEBUG_WARNING);
  if (strncmp(resource_locator, NACL_RESOURCE_DEBUG_WARNING,
              prefix_len) == 0) {
    allow_debug = 1;
    resource_locator += prefix_len;
  }

  for (ix = 0; ix < self->num_schemes; ++ix) {
    if (self->schemes[ix].default_scheme) {
      default_ix = ix;
    }
    prefix_len = strlen(self->schemes[ix].scheme_prefix);
    NaClLog(4, " prefix \"%s\"\n", self->schemes[ix].scheme_prefix);
    if (0 == strncmp(self->schemes[ix].scheme_prefix, resource_locator,
                     prefix_len)) {
      char const *rest = resource_locator + prefix_len;
      NaClLog(4, " prefix match at %"NACL_PRIuS", rest \"%s\".\n", ix, rest);
      return (*self->schemes[ix].Open)(self, rest,
                                       nacl_flags, mode, allow_debug);
    }
  }
  if (default_ix < self->num_schemes) {
    NaClLog(4, " trying default scheme %"NACL_PRIuS".\n", default_ix);
    return (*self->schemes[default_ix].Open)(self, resource_locator,
                                             nacl_flags, mode, allow_debug);
  }
  NaClLog(4, " no match, and no default scheme to try.");
  return NULL;
}

int NaClResourceCtor(struct NaClResource              *self,
                     struct NaClResourceSchemes const *scheme_tbl,
                     size_t                           num_schemes) {
  self->schemes = scheme_tbl;
  self->num_schemes = num_schemes;
  return 1;
}

/* --------------------------------------------------------------------------
 *
 * Subclass of NaClResource
 *
 * --------------------------------------------------------------------------
 */

int NaClResourceNaClAppCtor(struct NaClResourceNaClApp        *self,
                            struct NaClResourceSchemes const  *scheme_tbl,
                            size_t                            num_schemes,
                            struct NaClApp                    *nap) {
  NaClLog(4,
          ("NaClResourceNaClAppCtor, scheme_tbl 0x%"NACL_PRIxPTR","
           " size %"NACL_PRIuS".\n"),
          (uintptr_t) scheme_tbl, num_schemes);
  if (!NaClResourceCtor(&self->base, scheme_tbl, num_schemes)) {
    return 0;
  }
  self->nap = nap;
  return 1;
}

static struct NaClDesc *NaClResourceNullFactory(void) {
  struct NaClDescNull *null_desc = NULL;

  null_desc = malloc(sizeof *null_desc);
  if (NULL == null_desc) {
    return NULL;
  }
  if (!NaClDescNullCtor(null_desc)) {
    free(null_desc);
    null_desc = NULL;
  }
  return (struct NaClDesc *) null_desc;
}

static struct NaClDesc *NaClResourceFileFactory(char const *resource_locator,
                                                int nacl_flags,
                                                int mode) {
  struct NaClHostDesc         *hd = NULL;
  struct NaClDescIoDesc       *did = NULL;
  struct NaClDesc             *rv = NULL;

  hd = malloc(sizeof *hd);
  did = malloc(sizeof *did);
  if (NULL == hd || NULL == did) {
    goto done;
  }
  NaClLog(4,
          ("NaClResourceFileFactory: invoking NaClHostDescOpen on"
           " %s, flags 0x%x, mode 0%o\n"),
          resource_locator, nacl_flags, mode);
  if (0 != NaClHostDescOpen(hd, resource_locator, nacl_flags, mode)) {
    NaClLog(LOG_INFO,
            "NaClResourceFileFactory: NaClHostDescOpen failed\n");
    goto done;
  }
  if (!NaClDescIoDescCtor(did, hd)) {
    NaClLog(LOG_INFO,
            "NaClResourceFileFactory: NaClDescIoDescCtor failed\n");
    if (0 != NaClHostDescClose(hd)) {
      NaClLog(LOG_FATAL, "NaClResourceFileFactory: NaClHostDescClose failed\n");
    }
    goto done;
  }
  hd = NULL;  /* ownership passed into did */
  rv = (struct NaClDesc *) did;  /* success */
  did = NULL;
 done:
  free(hd);
  free(did);
  return rv;
}

static struct NaClDesc *NaClResourceNaClAppFileOpen(
    struct NaClResource *vself,
    char const *resource_locator,
    int nacl_flags,
    int mode,
    int allow_debug) {
  struct NaClDesc *rv = NULL;
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(allow_debug);

  if (0 == strcmp(resource_locator, NACL_RESOURCE_FILE_DEV_NULL)) {
    rv = NaClResourceNullFactory();
    if (NULL == rv) {
      NaClLog(LOG_ERROR, "Could not create Null device. Redirect failed.\n");
    }
  } else {
    rv = NaClResourceFileFactory(resource_locator, nacl_flags, mode);
    if (NULL == rv) {
      NaClLog(LOG_ERROR, "Could not open file \"%s\". Redirect failed.\n",
              resource_locator);
    }
  }
  NaClLog(4, "NaClResourceNaClAppFileOpen returning 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) rv);
  return rv;
}

int NaClResourceNaClAppInit(struct NaClResourceNaClApp  *rp,
                            struct NaClApp              *nap) {
  static struct NaClResourceSchemes const schemes[] = {
    {
      NACL_RESOURCE_FILE_PREFIX,
      1,  /* default scheme */
      NaClResourceNaClAppFileOpen,
    },
  };

  NaClLog(4, "NaClResourceNaClAppInit -- Ctor with default schemes\n");
  return NaClResourceNaClAppCtor(rp, schemes, NACL_ARRAY_SIZE(schemes), nap);
}
