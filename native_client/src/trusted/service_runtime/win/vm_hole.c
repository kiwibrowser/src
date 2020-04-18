/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"


void NaClVmHoleWaitToStartThread(struct NaClApp *nap) {
  NaClXMutexLock(&nap->mu);

  /* ensure no virtual memory hole may appear */
  while (nap->vm_hole_may_exist) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
  }

  ++nap->threads_launching;
  NaClXMutexUnlock(&nap->mu);
  /*
   * NB: Dropped lock, so many threads launching can starve VM
   * operations.  If this becomes a problem in practice, we can use a
   * reader/writer lock so that a waiting writer will block new
   * readers.
   */
}

void NaClVmHoleThreadStackIsSafe(struct NaClApp *nap) {
  NaClXMutexLock(&nap->mu);

  if (0 == --nap->threads_launching) {
    /*
     * Wake up the threads waiting to do VM operations.
     */
    NaClXCondVarBroadcast(&nap->cv);
  }

  NaClXMutexUnlock(&nap->mu);
}

/*
 * NaClVmHoleOpeningMu() is called when we are about to open a hole in
 * untrusted address space on Windows, where we cannot atomically
 * remap pages.
 *
 * NaClVmHoleOpeningMu() must be called with the mutex nap->mu held.
 * NaClVmHoleClosingMu() must later be called to undo the effects of
 * this call.
 */
void NaClVmHoleOpeningMu(struct NaClApp *nap) {
  /*
   * Temporarily stop any of NaCl's threads from launching so that no
   * trusted thread's stack will be allocated inside the mmap hole.
   */
  while (0 != nap->threads_launching) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
  }
  nap->vm_hole_may_exist = 1;

  /*
   * For safety, suspend all untrusted threads so that if another
   * trusted thread (outside of our control) allocates memory that is
   * placed into the mmap hole, untrusted code will not be able to
   * write to that location.
   */
  NaClUntrustedThreadsSuspendAll(nap, /* save_registers= */ 0);
}

/*
 * NaClVmHoleClosingMu() is the counterpart of NaClVmHoleOpeningMu().
 * It must be called with the mutex nap->mu held.
 */
void NaClVmHoleClosingMu(struct NaClApp *nap) {
  NaClUntrustedThreadsResumeAll(nap);

  nap->vm_hole_may_exist = 0;
  NaClXCondVarBroadcast(&nap->cv);
}
