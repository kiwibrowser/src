/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"

#if NACL_OSX
# include <pthread.h>
#endif

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/arch/x86/sel_ldr_x86.h"
#include "native_client/src/trusted/service_runtime/arch/x86/sel_rt.h"
#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"

static struct NaClMutex gNaClTlsMu;

static int gNaClThreadIdxInUse[NACL_THREAD_MAX];  /* bool */
static size_t const kNumThreads = NACL_ARRAY_SIZE_UNSAFE(gNaClThreadIdxInUse);

static void NaClThreadStartupCheck(void) {
  /*
   * really only needed for OSX -- make sure that the register
   * size does not exceed the size of a void*.
   */
  CHECK(sizeof(void *) >= sizeof ((struct NaClThreadContext *) 0)->prog_ctr);
}


/*
 * The thread indices are "virtual" segment selectors for x86-64.  In
 * x86-32, the allocation is done via the LDT, and a zero segment
 * selector is an illegal value.  Here, we emulate that -- though we
 * can have more than 4k threads -- and return NACL_TLS_INDEX_INVALID
 * (0) as an error.
 */

static int NaClThreadIdxInit (void) {
  size_t i;

  for (i = 0; i < kNumThreads; i++) {
    gNaClThreadIdxInUse[i] = 0;
  }
  if (!NaClMutexCtor(&gNaClTlsMu)) {
    NaClLog(LOG_WARNING,
            "NaClTlsInit: gNaClTlsMu initialization failed\n");
    return 0;
  }
  return 1;
}

static void NaClThreadIdxFini(void) {
  NaClMutexDtor(&gNaClTlsMu);
}

/*
 * Returns NACL_TLS_INDEX_INVALID (0) for error, [1, NACL_THREAD_MAX)
 * on success.  Since we are essentially allocating indices in an
 * array and array indexing is zero-based, we just skip the first
 * entry.
 */
static int NaClThreadIdxAllocate(void) {
  size_t i;

  NaClXMutexLock(&gNaClTlsMu);
  for (i = 1; i < kNumThreads; i++) {
    if (!gNaClThreadIdxInUse[i]) {
      gNaClThreadIdxInUse[i] = 1;
      break;
    }
  }
  NaClXMutexUnlock(&gNaClTlsMu);

  if (kNumThreads != i) {
    return (int) i;
  }
  NaClLog(LOG_ERROR, "NaClAllocateThreadIdx: no more slots for a thread\n");
  return NACL_TLS_INDEX_INVALID;
}

static void NaClThreadIdxFree(uint32_t i) {
  CHECK(i < kNumThreads);

  NaClXMutexLock(&gNaClTlsMu);
  gNaClThreadIdxInUse[i] = 0;
  NaClXMutexUnlock(&gNaClTlsMu);
}

/*
 * The API, defined for x86-32, requires that we return
 * NACL_TLS_INDEX_INVALID (0) for error.  This was because 0 is an
 * illegal segment selector value.
 *
 * Allocation does not mean we can set the TSD variable, since we are
 * not that thread.  The setting of the TSD must wait until the thread
 * actually launches.
 */
uint32_t NaClTlsAllocate(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);

  return NaClThreadIdxAllocate();
}

void NaClTlsFree(struct NaClAppThread *natp) {
  NaClThreadIdxFree(NaClGetThreadIdx(natp));
  /*
   * don't do
   *
   * pthread_setspecific(nacl_thread_info_key, (void *) 0);
   *
   * since this is invoked from the NaClAppThread's dtor, not by the
   * dying thread.  The thread is already dead and gone, so the TSD is
   * no longer available.
   */
}

void NaClTlsSetTlsValue1(struct NaClAppThread *natp, uint32_t value) {
  natp->user.tls_value1 = value;
}

void NaClTlsSetTlsValue2(struct NaClAppThread *natp, uint32_t value) {
  natp->user.tls_value2 = value;
}

uint32_t NaClTlsGetTlsValue1(struct NaClAppThread *natp) {
  return natp->user.tls_value1;
}

uint32_t NaClTlsGetTlsValue2(struct NaClAppThread *natp) {
  return natp->user.tls_value2;
}

uint32_t NaClGetThreadIdx(struct NaClAppThread *natp) {
  return natp->user.tls_idx;
}

#if NACL_OSX

static pthread_key_t nacl_thread_info_key;
uint32_t nacl_current_thread_tls_offset;

int NaClTlsInit(void) {
  int errnum;

  /*
   * Unlike Linux and Windows, Mac OS X does not provide TLS variables
   * that can be accessed via a TLS register without a function call,
   * which we need in order to restore the thread's trusted stack in
   * nacl_syscall_64.S.
   *
   * OS X only provides pthread_getspecific().  However, its code is
   * trivial, so we can inline it.  Of course, this assumes a
   * particular pthread implementation, so we check this assumption.
   * If the pthread implementation changes, we will need to update
   * this code.
   *
   * The current expected code is:
   *   65 48 8b 04 fd 60 00 00 00  movq %gs:_PTHREAD_TSD_OFFSET(,%rdi,8),%rax
   *   c3                          ret
   * in which _PTHREAD_TSD_OFFSET == 0x60.
   * But we accept other values of _PTHREAD_TSD_OFFSET below.
   */
  uint8_t *code = (uint8_t *) (uintptr_t) pthread_getspecific;
  uint32_t pthread_tsd_offset;
  if (!(code[0] == 0x65 &&
        code[1] == 0x48 &&
        code[2] == 0x8b &&
        code[3] == 0x04 &&
        code[4] == 0xfd &&
        code[9] == 0xc3)) {
    NaClLog(LOG_FATAL, "NaClTlsInit: Unexpected implementation of "
            "pthread_getspecific().  NaCl requires porting to the version "
            "of Mac OS X's pthread library on this system.\n");
  }
  pthread_tsd_offset = *(uint32_t *) (code + 5);
  NaClLog(2, "NaClTlsInit: Found expected implementation of "
          "pthread_getspecific(), and _PTHREAD_TSD_OFFSET=0x%"NACL_PRIx32"\n",
          pthread_tsd_offset);

  NaClThreadStartupCheck();

  if (!NaClThreadIdxInit()) {
    return 0;
  }

  errnum = pthread_key_create(&nacl_thread_info_key, (void (*)(void *)) 0);
  if (0 != errnum) {
    NaClLog(LOG_WARNING,
            "NaClTlsInit: pthread_key_create failed for thread info key, %d\n",
            errnum);
    return 0;
  }
  nacl_current_thread_tls_offset =
      pthread_tsd_offset + 8 * nacl_thread_info_key;

  return 1;
}

void NaClTlsFini(void) {
  int errnum = pthread_key_delete(nacl_thread_info_key);
  if (0 != errnum) {
    NaClLog(LOG_FATAL,
            "NaClTlsInit: pthread_key_delete failed for thread info key, %d\n",
            errnum);
  }
  NaClThreadIdxFini();
  return;
}

void NaClTlsSetCurrentThread(struct NaClAppThread *natp) {
  struct NaClThreadContext *ntcp_check;

  if (pthread_setspecific(nacl_thread_info_key, &natp->user) != 0) {
    NaClLog(LOG_FATAL, "NaClTlsSetCurrentThread: "
            "pthread_setspecific() failed\n");
  }

  /*
   * Sanity check:  Make sure that reading back the value using our
   * knowledge of Mac OS X's TLS internals gives us the correct value.
   * This checks that we inferred _PTHREAD_TSD_OFFSET correctly earlier.
   */
  __asm__("movq %%gs:(%1), %0"
          : "=r"(ntcp_check)
          : "r"(nacl_current_thread_tls_offset));
  if (ntcp_check != &natp->user) {
    NaClLog(LOG_FATAL, "NaClTlsSetCurrentThread: Sanity check failed: "
            "TLS offset must be wrong\n");
  }
}

/*
 * May be called from asm (or have compiler-generated code copied into
 * asm)!  We assume that register state is amenable to the use of
 * pthread_getspecific.  For both x86-32 and x86-64 OSX, this compiles
 * to just a memory load using the %gs or %fs segment prefixes, and
 * for x86-64 we do not change %fs.  (However, we are unlikely to use
 * this for x86-32 on OSX, since that's handled by NaCl "Classic"
 * where %gs gets swapped, and we use %gs >> 3 in the asm code.)
 */
struct NaClAppThread *NaClTlsGetCurrentThread(void) {
  struct NaClThreadContext *ntcp = pthread_getspecific(nacl_thread_info_key);
  return NaClAppThreadFromThreadContext(ntcp);
}

#elif NACL_LINUX || NACL_WINDOWS

/* May be NULL if the current thread does not host a NaClAppThread. */
THREAD struct NaClThreadContext *nacl_current_thread;

int NaClTlsInit(void) {
  NaClThreadStartupCheck();

  if (!NaClThreadIdxInit()) {
    return 0;
  }
  return 1;
}

void NaClTlsFini(void) {
  NaClThreadIdxFini();
}

/*
 * On x86-64, we must avoid using segment registers since Windows
 * Vista 64, Windows 7 64, etc do not permit user code to create LDT
 * entries.  The sandboxing scheme reserves %r15 for the base of the
 * NaCl app's 4G address space, but does not reserve any additional
 * registers for thread identity.  This reduces additional register
 * pressure, but implies that we have to figure out the thread
 * identity in some other way -- we use TLS (or TSD, see below) to do
 * so, and on context switch we must access the TLS variable in order
 * to determine where to save the user register context.
 */
void NaClTlsSetCurrentThread(struct NaClAppThread *natp) {
  nacl_current_thread = &natp->user;
}

struct NaClAppThread *NaClTlsGetCurrentThread(void) {
  return NaClAppThreadFromThreadContext(nacl_current_thread);
}

#else
# error "Woe to the service runtime.  What OS is it being compiled for?!?"
#endif
