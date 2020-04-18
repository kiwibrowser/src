/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG implementation.
 */
#include <windows.h>

/*
 * #define needed to link in RtlGenRandom(), a.k.a. SystemFunction036. See
 * the "Community Additions" comment on MSDN here:
 *   http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
 */
#define SystemFunction036 NTAPI SystemFunction036
#include <NTSecAPI.h>
#undef SystemFunction036

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_secure_random.h"


static void NaClSecureRngDtor(struct NaClSecureRngIf *vself);
static uint8_t NaClSecureRngGenByte(struct NaClSecureRngIf *vself);

static struct NaClSecureRngIfVtbl const kNaClSecureRngVtbl = {
  NaClSecureRngDtor,
  NaClSecureRngGenByte,
  NaClSecureRngDefaultGenUint32,
  NaClSecureRngDefaultGenBytes,
  NaClSecureRngDefaultUniform,
};

void NaClSecureRngModuleInit(void) {
  return;
}

void NaClSecureRngModuleFini(void) {
  return;
}

int NaClSecureRngCtor(struct NaClSecureRng *self) {
  self->base.vtbl = &kNaClSecureRngVtbl;
  self->nvalid = 0;
  return 1;
}

int NaClSecureRngTestingCtor(struct NaClSecureRng *self,
                             uint8_t              *seed_material,
                             size_t               seed_bytes) {
  UNREFERENCED_PARAMETER(seed_material);
  UNREFERENCED_PARAMETER(seed_bytes);
  self->base.vtbl = NULL;
  self->nvalid = 0;
  return 0;
}

static void NaClSecureRngDtor(struct NaClSecureRngIf *vself) {
  struct NaClSecureRng *self = (struct NaClSecureRng *) vself;
  SecureZeroMemory(self->buf, sizeof self->buf);
  vself->vtbl = NULL;
  return;
}

static void NaClSecureRngFilbuf(struct NaClSecureRng *self) {
  if (!RtlGenRandom(self->buf, sizeof self->buf)) {
    NaClLog(LOG_FATAL, "RtlGenRandom failed: error 0x%x\n", GetLastError());
  }
  self->nvalid = sizeof self->buf;
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
  return self->buf[--self->nvalid];
}
