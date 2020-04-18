/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Native Client pthreads implementation layer
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_PTHREAD_NC_PTHREAD_TYPES_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_PTHREAD_NC_PTHREAD_TYPES_H_ 1

#include <sys/queue.h>

#include "native_client/src/untrusted/pthread/pthread.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  THREAD_RUNNING,
  THREAD_TERMINATED
} nc_thread_status_t;

typedef struct entry {
  STAILQ_ENTRY(entry) entries;  /* a pointer - 4 bytes */
  int32_t is_used;              /* 4 bytes */
  int32_t size;                 /* 4 bytes */
  /* The whole structure should be 32 bytes to keep the alignment. */
  int32_t padding[5];              /* 4 * 5 = 20 bytes */
} nc_thread_memory_block_t;

struct __nc_basic_thread_data;

/* This struct defines the layout of the TDB */
typedef struct nc_thread_descriptor {
  void *tls_base;  /* tls accesses are made relative to this base */
  /*
   * joinable and join_waiting are always accessed with
   * __nc_thread_management_lock held (except when the thread is being
   * created).
   */
  int joinable;
  int join_waiting;
  nc_thread_memory_block_t *stack_node;
  void *tls_allocation;  /* Used for free()ing the TLS+TDB allocation. */
  void *(*start_func)(void *thread_arg);
  void *state;
  /*
   * irt_thread_data is used when libpthread is linked into the IRT.
   * It is used for free()ing the thread block.
   * TODO(mseaborn): This plays a similar role to tls_allocation; the
   * two could be unified in future.
   */
  void *irt_thread_data;
  struct __nc_basic_thread_data *basic_data;
} nc_thread_descriptor_t;

typedef struct __nc_basic_thread_data {
  void *retval;
  nc_thread_status_t status;
  pthread_cond_t join_condvar;
  /* Pointer to the tdb, will be null after the thread terminates. */
  nc_thread_descriptor_t *tdb;
} nc_basic_thread_data_t;

/*
 * This structure is allocated for all threads.  However, for threads
 * created with pthread_create(), the basic_data field is not used,
 * and a separate nc_basic_thread_data_t struct is allocated.  The
 * initial thread, and threads created by the IRT's thread_create()
 * interface, do use the basic_data field though.
 * TODO(mseaborn): Clean this up to be more consistent.
 */
struct nc_combined_tdb {
  nc_thread_descriptor_t tdb;
  nc_basic_thread_data_t basic_data;
};


#define MEMORY_BLOCK_ALLOCATION_SIZE(real_size) \
  (sizeof(nc_thread_memory_block_t) + (real_size))

static inline void *nc_memory_block_to_payload(nc_thread_memory_block_t *node) {
  return &node[1];
}

#ifdef __cplusplus
}
#endif

#endif  /* NATIVE_CLIENT_SRC_UNTRUSTED_PTHREAD_NC_PTHREAD_TYPES_H_ */
