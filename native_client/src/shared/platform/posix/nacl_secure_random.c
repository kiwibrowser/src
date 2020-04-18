/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG implementation.
 */

#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_secure_random.h"

#ifndef NACL_SECURE_RANDOM_SYSTEM_RANDOM_SOURCE
# define NACL_SECURE_RANDOM_SYSTEM_RANDOM_SOURCE "/dev/urandom"
#endif

static struct NaClSecureRngIfVtbl const kNaClSecureRngVtbl;

/* use -1 to ensure a fast failure if module initializer is not called */
static int  urandom_d = -1;

/*
 * This sets a /dev/urandom file descriptor for this module to use.
 * This is for use inside outer sandboxes where opening /dev/urandom
 * with open() does not work.
 *
 * This function should be called before NaClSecureRngModuleInit() so
 * that we do not attempt to use open() inside the outer sandbox.
 *
 * This takes ownership of the file descriptor.
 */
void NaClSecureRngModuleSetUrandomFd(int fd) {
  CHECK(urandom_d == -1);
  urandom_d = fd;
}

void NaClSecureRngModuleInit(void) {
  /*
   * Check whether we have already been initialised via
   * NaClSecureRngModuleSetUrandomFd().
   */
  if (urandom_d != -1) {
    return;
  }

  urandom_d = open(NACL_SECURE_RANDOM_SYSTEM_RANDOM_SOURCE, O_RDONLY, 0);
  if (-1 == urandom_d) {
    NaClLog(LOG_FATAL, "Cannot open system random source %s\n",
            NACL_SECURE_RANDOM_SYSTEM_RANDOM_SOURCE);
  }
}

void NaClSecureRngModuleFini(void) {
  if (urandom_d != -1) {
    if (close(urandom_d) != 0) {
      NaClLog(LOG_FATAL,
              "NaClSecureRngModuleFini: close() failed with errno %d\n",
              errno);
    }
    urandom_d = -1;
  }
}

int NaClSecureRngCtor(struct NaClSecureRng *self) {
  self->base.vtbl = &kNaClSecureRngVtbl;
  self->nvalid = 0;
  return 1;
}

int NaClSecureRngTestingCtor(struct NaClSecureRng *self,
                             uint8_t              *seed_material,
                             size_t               seed_bytes) {
  UNREFERENCED_PARAMETER(self);
  UNREFERENCED_PARAMETER(seed_material);
  UNREFERENCED_PARAMETER(seed_bytes);
  return 0;
}

static void NaClSecureRngDtor(struct NaClSecureRngIf *vself) {
  struct NaClSecureRng *self = (struct NaClSecureRng *) vself;
  memset(self->buf, 0, sizeof self->buf);
  vself->vtbl = NULL;
  return;
}

static void NaClSecureRngFilbuf(struct NaClSecureRng *self) {
  VCHECK(-1 != urandom_d,
         ("NaClSecureRngCtor: random descriptor invalid;"
          " module initialization failed?\n"));
  self->nvalid = read(urandom_d, self->buf, sizeof self->buf);
  if (self->nvalid <= 0) {
    NaClLog(LOG_FATAL, "NaClSecureRngFilbuf failed, read returned %d\n",
            self->nvalid);
  }
}

static uint8_t NaClSecureRngGenByte(struct NaClSecureRngIf *vself) {
  struct NaClSecureRng *self = (struct NaClSecureRng *) vself;

  if (0 > self->nvalid) {
    NaClLog(LOG_FATAL,
            "NaClSecureRngGenByte: illegal buffer state, nvalid = %d\n",
            self->nvalid);
  }
  if (0 == self->nvalid) {
    NaClSecureRngFilbuf(self);
  }
  /* 0 < self->nvalid <= sizeof self->buf */
  return self->buf[--self->nvalid];
}

static struct NaClSecureRngIfVtbl const kNaClSecureRngVtbl = {
  NaClSecureRngDtor,
  NaClSecureRngGenByte,
  NaClSecureRngDefaultGenUint32,
  NaClSecureRngDefaultGenBytes,
  NaClSecureRngDefaultUniform,
};
