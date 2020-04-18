/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Directory descriptor abstraction.
 */

#include "native_client/src/include/portability.h"

#include <stdlib.h>
#include <string.h>

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_invalid.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * Externalizing the invalid descriptor -- which shows up in untrusted
 * user code as -1 -- loses flags and metadata.  No operations are
 * ever be allowed for the invalid descriptor (the vtable entries are
 * all *Unimplemented), so there is no opportunity for code to look at
 * flags/metadata.
 */
static int NaClDescInvalidExternalizeSize(struct NaClDesc      *vself,
                                          size_t               *nbytes,
                                          size_t               *nhandles) {
  UNREFERENCED_PARAMETER(vself);
  *nbytes = 0;
  *nhandles = 0;
  return 0;
}

static int NaClDescInvalidExternalize(struct NaClDesc          *vself,
                                      struct NaClDescXferState *xfer) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(xfer);
  return 0;
}

/*
 * NaClDescInvalid is the subclass for the singleton invalid descriptor.
 */

static struct NaClDescVtbl const kNaClDescInvalidVtbl = {
  {
    NaClDescDtorNotImplemented,
  },
  NaClDescMapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescPReadNotImplemented,
  NaClDescPWriteNotImplemented,
  NaClDescFstatNotImplemented,
  NaClDescFchdirNotImplemented,
  NaClDescFchmodNotImplemented,
  NaClDescFsyncNotImplemented,
  NaClDescFdatasyncNotImplemented,
  NaClDescFtruncateNotImplemented,
  NaClDescGetdentsNotImplemented,
  NaClDescInvalidExternalizeSize,
  NaClDescInvalidExternalize,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescLowLevelSendMsgNotImplemented,
  NaClDescLowLevelRecvMsgNotImplemented,
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
  NaClDescSetMetadata,
  NaClDescGetMetadata,
  NaClDescSetFlags,
  NaClDescGetFlags,
  NaClDescIsattyNotImplemented,
  NACL_DESC_INVALID,
};

static struct NaClMutex *mutex = NULL;
static struct NaClDescInvalid *singleton;

void NaClDescInvalidInit(void) {
  mutex = (struct NaClMutex *) malloc(sizeof(*mutex));
  if (NULL == mutex) {
    NaClLog(LOG_FATAL, "Cannot allocate NaClDescInvalid mutex\n");
  }
  if (!NaClMutexCtor(mutex)) {
    free(mutex);
    mutex = NULL;
    NaClLog(LOG_FATAL, "Cannot construct NaClDescInvalid mutex\n");
  }
}

void NaClDescInvalidFini(void) {
  if (NULL != mutex) {
    NaClMutexDtor(mutex);
    free(mutex);
    mutex = NULL;
  }
}

struct NaClDescInvalid const *NaClDescInvalidMake(void) {
  NaClXMutexLock(mutex);
  if (NULL == singleton) {
    do {
      /* Allocate an instance. */
      singleton = (struct NaClDescInvalid *) malloc(sizeof(*singleton));
      if (NULL == singleton) {
        break;
      }
      /* Do the base class construction. */
      if (!NaClDescCtor(&(singleton->base))) {
        free(singleton);
        singleton = NULL;
        break;
      }
      /* Construct the derived class (simply set the vtbl). */
      singleton->base.base.vtbl =
          (struct NaClRefCountVtbl const *) &kNaClDescInvalidVtbl;
    } while (0);
  }
  NaClXMutexUnlock(mutex);
  /* If we reached this point and still have NULL == singleton there was an
   * error in allocation or construction.  Return NULL to indicate error.
   */
  if (NULL == singleton) {
    return NULL;
  }

  return (struct NaClDescInvalid *) NaClDescRef(&(singleton->base));
}

int NaClDescInvalidInternalize(struct NaClDesc               **baseptr,
                               struct NaClDescXferState      *xfer) {
  UNREFERENCED_PARAMETER(xfer);

  *baseptr = (struct NaClDesc *) NaClDescInvalidMake();

  return 0;
}
