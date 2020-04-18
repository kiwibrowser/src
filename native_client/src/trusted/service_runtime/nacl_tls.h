/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SERVICE_RUNTIME_NACL_THREAD_H__
#define SERVICE_RUNTIME_NACL_THREAD_H__ 1

#include "native_client/src/include/build_config.h"
/*
 * This header contains the prototypes for thread/tls related
 * functions whose implementation is highly architecture/platform
 * specific.
 *
 * The function primarily adress complications stemming from the
 * x86-32 segmentation model - on arm things are somewhat simpler.
 *
 * On x86-64, no segment is used either.  This API is (ab)used to
 * stash the tdb in a thread-local variable (TLS on NACL_LINUX and
 * NACL_WINDOWS, and TSD on NACL_OSX since OSX does not implement
 * __thread).
 */
#include "native_client/src/include/portability.h"

struct NaClAppThread;

int NaClTlsInit(void);

void NaClTlsFini(void);


#define NACL_TLS_INDEX_INVALID 0

/*
 * Allocates a thread index for the thread.  On x86-32, the thread
 * index is the gs segment number.  On x86-64 and ARM, the thread
 * index is used internally in NaCl but has no other meaning.
 * This is called for the main thread and all subsequent threads
 * being created via NaClAppThreadMake().
 * On error, returns NACL_TLS_INDEX_INVALID.
 */
uint32_t NaClTlsAllocate(struct NaClAppThread *natp) NACL_WUR;

/*
 * Free a tls descriptor.
 * This is called from NaClAppThreadDelete() which in turn is called
 * after a thread terminates.
 */
void NaClTlsFree(struct NaClAppThread *natp);


/*
 * Called in thread bootup code, to set TLS/TSD when the thread ID is not
 * saved in a reserved register (e.g., %gs in NaCl x86-32).
 */
void NaClTlsSetCurrentThread(struct NaClAppThread *natp);

/*
 * Get the current thread as set by NaClTlsSetCurrentThread().
 */
struct NaClAppThread *NaClTlsGetCurrentThread(void);

void NaClTlsSetTlsValue1(struct NaClAppThread *natp, uint32_t value);
void NaClTlsSetTlsValue2(struct NaClAppThread *natp, uint32_t value);

uint32_t NaClTlsGetTlsValue1(struct NaClAppThread *natp);
uint32_t NaClTlsGetTlsValue2(struct NaClAppThread *natp);

/*
 * Get the current thread index which is used to look up information in a
 * number of internal structures, e.g. nacl_user[], nacl_thread_ids[]
 */
uint32_t NaClGetThreadIdx(struct NaClAppThread *natp);


#endif /* SERVICE_RUNTIME_NACL_THREAD_H__ */
