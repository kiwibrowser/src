/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"

/*
 * For Posix, we use sigsetjmp and siglongjmp as a means to safely return
 * from the signal handler instead of re-executing the same code.
 */
static sigjmp_buf try_state;
#define SIG_TRY_START   if (0 == sigsetjmp(try_state, 1)) {
#define SIG_TRY_END     }

int g_SigFound;

typedef union {
  void (*func)(void);
  void *data;
} PtrUnion_t;


/*
 * Allocate one page with the specified protection attributes, or exit.
 */
static void *Alloc1Page(int flags) {
  void *page;
  if (NaClPageAlloc(&page, NACL_PAGESIZE)) {
    printf("Failed to allocate page.\n");
    exit(-1);
  }

  if (NaClMprotect(page, NACL_PAGESIZE, flags)) {
    printf("Failed to set page protection to %d.\n", flags);
    exit(-1);
  }

  return page;
}

static void Free1Page(void *page) {
  NaClPageFree(page, NACL_PAGESIZE);
}

/* Execute a non execute page. */
void WINAPI Exec_RW(void *ptr) {
  void *page;

  PtrUnion_t ptrUnion;

  UNREFERENCED_PARAMETER(ptr);

  /* Allocate a READ/WRITE page */
  page = Alloc1Page(PROT_READ | PROT_WRITE);

  /* Try to execute it */
  SIG_TRY_START
    ptrUnion.data = page;
    ptrUnion.func();
    g_SigFound = 1;
  SIG_TRY_END
  printf("Done.\n");
}

/* Write a non-writable page. */
void WINAPI Write_RX(void *ptr) {
  void *page;

  UNREFERENCED_PARAMETER(ptr);
  /* Allocate a READ/EXEC page */
  page = Alloc1Page(PROT_READ | PROT_EXEC);

  /* Try to write it */
  SIG_TRY_START
    *((intptr_t *) page) = 0;
    g_SigFound = 1;
  SIG_TRY_END

  Free1Page(page);
  printf("Done.\n");
}

/* Read/Modify/Write an unmapped page. */
void WINAPI ReadWriteUnmapped(void *ptr) {
  int *page;

  UNREFERENCED_PARAMETER(ptr);

  /* Allocate and free so we know it can't be valid. */
  page = (int *) Alloc1Page(PROT_READ | PROT_WRITE);
  Free1Page(page);

  /* Try to read modify write the freed page */
  SIG_TRY_START
    page[0]++;
    g_SigFound = 1;
  SIG_TRY_END

  printf("Done.\n");
}

/* Divide by Zero */
void WINAPI DivZero(void *ptr) {
  intptr_t a = 100;

  /* Bypass div by zero warning. */
  SIG_TRY_START
    intptr_t b = (intptr_t) ptr;
    if ((a / b) != 0) printf("DIVIDED!\n");
    g_SigFound = 1;
  SIG_TRY_END
  printf("Done.\n");
}


void Attempt(const char *str, void (WINAPI *start_fn)(void *), int sig) {
  struct NaClThread thread;
  printf("Waiting for %d on %s.\n", sig, str);
  g_SigFound = 0;

  if (!NaClThreadCreateJoinable(&thread, start_fn, NULL, 65536)) {
    NaClLog(LOG_FATAL, "Error: Attempt() could not create thread\n");
  }
  NaClThreadJoin(&thread);

  if (sig == g_SigFound) return;

  /* If we have not matched, then print an error and exit. */
  printf("Error: Got signal %d, while expecting %d.\n", g_SigFound, sig);
  exit(-1);
}

void Handler(int signal_number, const struct NaClSignalContext *context,
             int is_untrusted) {
  UNREFERENCED_PARAMETER(context);
  UNREFERENCED_PARAMETER(is_untrusted);

  g_SigFound = signal_number;

  siglongjmp(try_state, 1);
}

#define ATTEMPT(x,sig) Attempt(#x,x,sig)
int main(int argc, const char *argv[]) {
  /*
   * There are inconsistencies between POSIX implementations on which errors
   * return SIGSEGV vs SIGBUS.
   */
#if NACL_OSX
  const int kAccessViolationOnMappedPageSignal = SIGBUS;
#else
  const int kAccessViolationOnMappedPageSignal = SIGSEGV;
#endif
  const int kAccessUnmappedPageSignal = SIGSEGV;
  const int kDivideByZeroSignal = SIGFPE;

  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  NaClLogModuleInit();
  NaClTimeInit();
  NaClInitGlobals();
  NaClTlsInit();
  NaClSignalHandlerInit();

  NaClSignalHandlerSet(Handler);

  ATTEMPT(Exec_RW, kAccessViolationOnMappedPageSignal);
  ATTEMPT(Write_RX, kAccessViolationOnMappedPageSignal);
  ATTEMPT(ReadWriteUnmapped, kAccessUnmappedPageSignal);
  ATTEMPT(DivZero, kDivideByZeroSignal);

  /* Correct shutdown order would have been:
   * NaClSignalHandlerFini();
   * NaClTimeFini();
   * NaClLogModuleInit();
   */

  return 0;
}
