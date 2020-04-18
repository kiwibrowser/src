/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Native Client threads library
 */

#include <limits.h>
#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/unistd.h>

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/tls.h"
#include "native_client/src/untrusted/nacl/tls_params.h"
#include "native_client/src/untrusted/pthread/pthread.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"
#include "native_client/src/untrusted/pthread/pthread_types.h"

#include "native_client/src/untrusted/valgrind/dynamic_annotations.h"

#if defined(NACL_IN_IRT)
# include "native_client/src/untrusted/irt/irt_private.h"
#endif

#define TDB_SIZE (sizeof(struct nc_combined_tdb))

/*
 * ABI tables for underyling NaCl thread interfaces. This is declared to be
 * global so that a user will be able to override it using the irt_ext API.
 */
struct nacl_irt_thread __libnacl_irt_thread;

/*
 * These days, the thread_create() syscall/IRT call will align the
 * stack for us, but for compatibility with older, released x86
 * versions of NaCl where thread_create() does not align the stack, we
 * align the stack ourselves.
 */
#if defined(__i386__)
static const uint32_t kStackAlignment = 32;
static const uint32_t kStackPadBelowAlign = 4;  /* Return address size */
#elif defined(__x86_64__)
static const uint32_t kStackAlignment = 32;
static const uint32_t kStackPadBelowAlign = 8;  /* Return address size */
#else
static const uint32_t kStackAlignment = 1;
static const uint32_t kStackPadBelowAlign = 0;
#endif

typedef struct nc_thread_cleanup_handler {
  struct nc_thread_cleanup_handler *previous;
  void (*handler_function)(void *arg);
  void *handler_arg;
} nc_thread_cleanup_handler;

static __thread nc_thread_cleanup_handler *__nc_cleanup_handlers = NULL;

static inline char *align(uint32_t offset, uint32_t alignment) {
  return (char *) ((offset + alignment - 1) & ~(alignment - 1));
}

/* Thread management global variables. */
static const int __nc_kMaxCachedMemoryBlocks = 50;

int __nc_thread_initialized;

/* Mutex used to synchronize thread management code. */
static pthread_mutex_t __nc_thread_management_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Condition variable that gets signaled when all the threads
 * except the main thread have terminated.
 */
static pthread_cond_t __nc_last_thread_cond = PTHREAD_COND_INITIALIZER;
pthread_t __nc_initial_thread_id;

/* Number of threads currently running in this NaCl module. */
static int __nc_running_threads_counter = 1;

/*
 * This is a list of memory blocks that were allocated for use as thread
 * stacks.  These correspond to threads that have either exited or are just
 * about to exit.  We maintain this list for two reasons:
 *
 *  * The main reason is that pthread_exit() can't deallocate the stack
 *    itself while it's running on that stack.  The stack can only be freed
 *    or reused after the "is_used" field gets set to zero by
 *    thread_exit().
 *
 *  * A secondary reason is that avoiding free()ing these blocks might be
 *    faster or might prevent memory fragmentation.
 */
static STAILQ_HEAD(tailhead, entry) __nc_thread_stack_blocks;
/* Number of entries in __nc_thread_stack_blocks. */
static int __nc_thread_stack_blocks_count;

/* Internal functions */

static inline struct nc_thread_descriptor *nc_get_tdb(void) {
  /*
   * Fetch the thread-specific data pointer.  This is usually just
   * a wrapper around __libnacl_irt_tls.tls_get() but we don't use
   * that here so that the IRT build can override the definition.
   */
  return (void *) ((char *) __nacl_read_tp_inline()
                   + __nacl_tp_tdb_offset(TDB_SIZE));
}

static void nc_thread_starter(void) {
  nc_thread_descriptor_t *tdb = nc_get_tdb();
  __newlib_thread_init();
#if defined(NACL_IN_IRT)
  g_is_irt_internal_thread = 1;
#endif
  void *retval = tdb->start_func(tdb->state);

  /*
   * Free handler list to prevent memory leak in case function returns
   * without calling pthread_cleanup_pop(), although doing that is unspecified
   * behaviour.
   */
  while (NULL != __nc_cleanup_handlers) {
    pthread_cleanup_pop(0);
  }

  /* If the function returns, terminate the thread. */
  pthread_exit(retval);
  /* NOTREACHED */
  /* TODO(gregoryd) - add assert */
}

static nc_thread_memory_block_t *nc_allocate_memory_block_mu(
    int required_size) {
  struct tailhead *head;
  nc_thread_memory_block_t *node;
  /* Assume the lock is held!!! */
  head = &__nc_thread_stack_blocks;

  if (!STAILQ_EMPTY(head)) {
    /* Try to get one from queue. */
    nc_thread_memory_block_t *node = STAILQ_FIRST(head);

    /*
     * On average the memory blocks will be marked as not used in the same order
     * as they are added to the queue, therefore there is no need to check the
     * next queue entries if the first one is still in use.
     */
    if (0 == node->is_used && node->size >= required_size) {
      /*
       * This will only re-use the first node possibly, and could be
       * improved to provide the stack with a best-fit algorithm if needed.
       * TODO: we should scan all nodes to see if there is one that fits
       *   before allocating another.
       *   http://code.google.com/p/nativeclient/issues/detail?id=1569
       */
      int size = node->size;
      STAILQ_REMOVE_HEAD(head, entries);
      --__nc_thread_stack_blocks_count;

      memset(node, 0,sizeof(*node));
      node->size = size;
      node->is_used = 1;
      return node;
    }

    while (__nc_thread_stack_blocks_count > __nc_kMaxCachedMemoryBlocks) {
      /*
       * We have too many blocks in the queue - try to release some.
       * The maximum number of memory blocks to keep in the queue
       * is almost arbitrary and can be tuned.
       * The main limitation is that if we keep too many
       * blocks in the queue, the NaCl app will run out of memory,
       * since the default thread stack size is 512K.
       * TODO(gregoryd): we might give up reusing stack entries once we
       * support variable stack size.
       */
      nc_thread_memory_block_t *tmp = STAILQ_FIRST(head);
      if (0 == tmp->is_used) {
        STAILQ_REMOVE_HEAD(head, entries);
        --__nc_thread_stack_blocks_count;
        free(tmp);
      } else {
        /*
         * Stop once we find a block that is still in use,
         * since probably there is no point to continue.
         */
        break;
      }
    }

  }
  /* No available blocks of the required type/size - allocate one. */
  node = malloc(MEMORY_BLOCK_ALLOCATION_SIZE(required_size));
  if (NULL != node) {
    memset(node, 0, sizeof(*node));
    node->size = required_size;
    node->is_used = 1;
  }
  return node;
}

static void nc_free_memory_block_mu(nc_thread_memory_block_t *node) {
  /* Assume the lock is held!!! */
  struct tailhead *head = &__nc_thread_stack_blocks;
  STAILQ_INSERT_TAIL(head, node, entries);
  ++__nc_thread_stack_blocks_count;
}

static void nc_release_basic_data_mu(nc_basic_thread_data_t *basic_data) {
  /* join_condvar can be initialized only if tls_allocation exists. */
  pthread_cond_destroy(&basic_data->join_condvar);
  free(basic_data);
}

static void nc_release_tls_allocation(void *tls_allocation,
                                      nc_thread_descriptor_t *tdb) {
  if (tls_allocation) {
    if (NULL != tdb->basic_data) {
      tdb->basic_data->tdb = NULL;
    }
    free(tls_allocation);
  }
}

/* Initialize a newly allocated TDB to some default values. */
static void nc_tdb_init(nc_thread_descriptor_t *tdb,
                        nc_basic_thread_data_t *basic_data) {
  tdb->tls_base = tdb;
  tdb->joinable = PTHREAD_CREATE_JOINABLE;
  tdb->join_waiting = 0;
  tdb->stack_node = NULL;
  tdb->tls_allocation = NULL;
  tdb->start_func = NULL;
  tdb->state = NULL;
  tdb->irt_thread_data = NULL;
  tdb->basic_data = basic_data;

  basic_data->retval = NULL;
  basic_data->status = THREAD_RUNNING;
  if (pthread_cond_init(&basic_data->join_condvar, NULL) != 0)
    __builtin_trap();
  basic_data->tdb = tdb;
}

/* Initializes all globals except for the initial thread structure. */
void __nc_initialize_globals(void) {
  /*
   * Fetch the ABI tables from the IRT.  If we don't have these, all is lost.
   */
  __nc_initialize_interfaces();

  /*
   * Tell ThreadSanitizer to not generate happens-before arcs between uses of
   * this mutex. Otherwise we miss to many real races.
   * When not running under ThreadSanitizer, this is just a call to an empty
   * function.
   */
  ANNOTATE_NOT_HAPPENS_BEFORE_MUTEX(&__nc_thread_management_lock);

  STAILQ_INIT(&__nc_thread_stack_blocks);

  __nc_thread_initialized = 1;
}

/*
 * This is used by the IRT for user threads.  We initialize all fields
 * so that we get predictable behaviour in case some IRT code does an
 * unsupported pthread operation on a user thread.
 */
void __nc_initialize_unjoinable_thread(struct nc_combined_tdb *tdb) {
  nc_tdb_init(&tdb->tdb, &tdb->basic_data);
  tdb->tdb.joinable = 0;
}

#if !defined(NACL_IN_IRT)

/*
 * Will be called from the library startup code,
 * which always happens on the application's main thread.
 */
void __pthread_initialize(void) {
  __pthread_initialize_minimal(TDB_SIZE);

  struct nc_combined_tdb *tdb = (struct nc_combined_tdb *) nc_get_tdb();
  nc_tdb_init(&tdb->tdb, &tdb->basic_data);
  __nc_initial_thread_id = &tdb->basic_data;

  __nc_initialize_globals();
}

#endif


/* pthread functions */

int pthread_create(pthread_t *thread_id,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *arg) {
  int retval = EAGAIN;
  void *esp;
  /* Declare the variables outside of the while scope. */
  nc_thread_memory_block_t *stack_node = NULL;
  char *thread_stack = NULL;
  nc_thread_descriptor_t *new_tdb = NULL;
  nc_basic_thread_data_t *new_basic_data = NULL;
  void *tls_allocation = NULL;
  size_t stacksize = PTHREAD_STACK_DEFAULT;
  void *new_tp;

  /* TODO(gregoryd) - right now a single lock is used, try to optimize? */
  pthread_mutex_lock(&__nc_thread_management_lock);

  do {
    /* Allocate the combined TLS + TDB block---see tls.h for explanation. */

    tls_allocation = malloc(__nacl_tls_combined_size(TDB_SIZE));
    if (NULL == tls_allocation)
      break;

    new_tp = __nacl_tls_initialize_memory(tls_allocation, TDB_SIZE);

    new_tdb = (nc_thread_descriptor_t *)
              ((char *) new_tp + __nacl_tp_tdb_offset(TDB_SIZE));

    new_basic_data = malloc(sizeof(*new_basic_data));
    if (NULL == new_basic_data) {
      /*
       * The tdb should be zero intialized.
       * This just re-emphasizes this requirement.
       */
      new_tdb->basic_data = NULL;
      break;
    }

    nc_tdb_init(new_tdb, new_basic_data);
    new_tdb->tls_allocation = tls_allocation;

    /*
     * All the required members of the tdb must be initialized before
     * the thread is started and actually before the global lock is released,
     * since another thread can call pthread_join() or pthread_detach().
     */
    new_tdb->start_func = start_routine;
    new_tdb->state = arg;
    if (attr != NULL) {
      new_tdb->joinable = attr->joinable;
      stacksize = attr->stacksize;
    }

    /* Allocate the stack for the thread. */
    stack_node = nc_allocate_memory_block_mu(stacksize + kStackAlignment - 1);
    if (NULL == stack_node) {
      retval = EAGAIN;
      break;
    }
    thread_stack = align((uint32_t) nc_memory_block_to_payload(stack_node),
                         kStackAlignment);
    new_tdb->stack_node = stack_node;

    retval = 0;
  } while (0);

  if (0 != retval) {
    pthread_mutex_unlock(&__nc_thread_management_lock);
    goto ret; /* error */
  }

  /*
   * Speculatively increase the thread count.  If thread creation
   * fails, we will decrease it back.  This way the thread count will
   * never be lower than the actual number of threads, but can briefly
   * be higher than that.
   */
  ++__nc_running_threads_counter;

  /*
   * Save the new thread id.  This can not be done after the syscall,
   * because the child thread could have already finished by that
   * time.  If thread creation fails, it will be overriden with -1
   * later.
   */
  *thread_id = new_basic_data;

  pthread_mutex_unlock(&__nc_thread_management_lock);

  /*
   * Calculate the top-of-stack location.  The very first location is a
   * zero address of architecture-dependent width, needed to satisfy the
   * normal ABI alignment requirements for the stack.  (On some machines
   * this is the dummy return address of the thread-start function.)
   *
   * Both thread_stack and stacksize are multiples of 16.
   */
  esp = (void *) (thread_stack + stacksize - kStackPadBelowAlign);
  memset(esp, 0, kStackPadBelowAlign);

  /* Start the thread. */
  retval = __libnacl_irt_thread.thread_create(nc_thread_starter, esp, new_tp);
  if (0 != retval) {
    pthread_mutex_lock(&__nc_thread_management_lock);
    /* TODO(gregoryd) : replace with atomic decrement? */
    --__nc_running_threads_counter;
    pthread_mutex_unlock(&__nc_thread_management_lock);
    goto ret;
  }

  assert(0 == retval);

ret:
  if (0 != retval) {
    /* Failed to create a thread. */
    pthread_mutex_lock(&__nc_thread_management_lock);

    nc_release_tls_allocation(tls_allocation, new_tdb);
    if (new_basic_data) {
      nc_release_basic_data_mu(new_basic_data);
    }
    if (stack_node) {
      stack_node->is_used = 0;
      nc_free_memory_block_mu(stack_node);
    }

    pthread_mutex_unlock(&__nc_thread_management_lock);
    *thread_id = NACL_PTHREAD_ILLEGAL_THREAD_ID;
  }

  return retval;
}

static int wait_for_threads(void) {
  pthread_mutex_lock(&__nc_thread_management_lock);

  while (1 != __nc_running_threads_counter) {
    pthread_cond_wait(&__nc_last_thread_cond, &__nc_thread_management_lock);
  }
  ANNOTATE_CONDVAR_LOCK_WAIT(&__nc_last_thread_cond,
                             &__nc_thread_management_lock);

  pthread_mutex_unlock(&__nc_thread_management_lock);
  return 0;
}

void pthread_cleanup_push(void (*routine)(void *), void *arg) {
  nc_thread_cleanup_handler *handler =
      (nc_thread_cleanup_handler *)malloc(sizeof(*handler));
  handler->handler_function = routine;
  handler->handler_arg = arg;
  handler->previous = __nc_cleanup_handlers;
  __nc_cleanup_handlers = handler;
}

void pthread_cleanup_pop(int execute) {
  if (NULL != __nc_cleanup_handlers) {
    nc_thread_cleanup_handler *handler = __nc_cleanup_handlers;
    __nc_cleanup_handlers = handler->previous;
    if (execute)
      handler->handler_function(handler->handler_arg);
    free(handler);
  }
}

void pthread_exit(void *retval) {
  /* Get all we need from the tdb before releasing it. */
  nc_thread_descriptor_t    *tdb = nc_get_tdb();
  nc_thread_memory_block_t  *stack_node = tdb->stack_node;
  int32_t                   *is_used = &stack_node->is_used;
  nc_basic_thread_data_t    *basic_data = tdb->basic_data;

  /* Call cleanup handlers. */
  while (NULL != __nc_cleanup_handlers) {
    pthread_cleanup_pop(1);
  }

  /* Call the destruction functions for TSD. */
  __nc_tsd_exit();

  __newlib_thread_exit();

  if (__nc_initial_thread_id != basic_data) {
    pthread_mutex_lock(&__nc_thread_management_lock);
    --__nc_running_threads_counter;
    pthread_mutex_unlock(&__nc_thread_management_lock);
  } else {
    /* This is the main thread - wait for other threads to complete. */
    wait_for_threads();
    exit(0);
  }

  pthread_mutex_lock(&__nc_thread_management_lock);

  basic_data->retval = retval;

  int joinable = tdb->joinable;
  if (joinable) {
    /* If somebody is waiting for this thread, signal. */
    basic_data->status = THREAD_TERMINATED;
    pthread_cond_signal(&basic_data->join_condvar);
  }
  /*
   * We can release TLS+TDB - thread id and its return value are still
   * kept in basic_data.
   */
  nc_release_tls_allocation(tdb->tls_allocation, tdb);

  if (!joinable) {
    nc_release_basic_data_mu(basic_data);
  }

  /* Now add the stack to the list but keep it marked as used. */
  nc_free_memory_block_mu(stack_node);

  if (1 == __nc_running_threads_counter) {
    pthread_cond_signal(&__nc_last_thread_cond);
  }

  pthread_mutex_unlock(&__nc_thread_management_lock);
  __libnacl_irt_thread.thread_exit(is_used);
  __builtin_trap();
}

int pthread_join(pthread_t thread_id, void **thread_return) {
  int retval = 0;
  nc_basic_thread_data_t *basic_data = thread_id;
  if (pthread_self() == thread_id) {
    return EDEADLK;
  }

  pthread_mutex_lock(&__nc_thread_management_lock);

  if (basic_data->tdb != NULL) {
    /* The thread is still running. */
    nc_thread_descriptor_t *joined_tdb = basic_data->tdb;
    if (!joined_tdb->joinable || joined_tdb->join_waiting) {
      /* The thread is detached or another thread is waiting to join. */
      retval = EINVAL;
      goto ret;
    }
    joined_tdb->join_waiting = 1;
    /* Wait till the thread terminates. */
    while (THREAD_TERMINATED != basic_data->status) {
      pthread_cond_wait(&basic_data->join_condvar,
                        &__nc_thread_management_lock);
    }
  }
  ANNOTATE_CONDVAR_LOCK_WAIT(&basic_data->join_condvar,
                             &__nc_thread_management_lock);
  /* The thread has already terminated. */
  /* Save the return value. */
  if (thread_return != NULL) {
    *thread_return = basic_data->retval;
  }

  /* Release the resources. */
  nc_release_basic_data_mu(basic_data);
  retval = 0;

ret:
  pthread_mutex_unlock(&__nc_thread_management_lock);

  return retval;

}

int pthread_detach(pthread_t thread_id) {
  int retval = 0;
  nc_basic_thread_data_t *basic_data = thread_id;
  nc_thread_descriptor_t *detached_tdb;
  /*
   * TODO(gregoryd) - can be optimized using InterlockedExchange
   * once it's available.
   */
  pthread_mutex_lock(&__nc_thread_management_lock);
  detached_tdb = basic_data->tdb;

  if (NULL == detached_tdb) {
    /* The thread has already terminated. */
    nc_release_basic_data_mu(basic_data);
  } else {
    if (!detached_tdb->join_waiting) {
      if (detached_tdb->joinable) {
        detached_tdb->joinable = 0;
      } else {
        /* Already detached. */
        retval = EINVAL;
      }
    } else {
      /* Another thread is already waiting to join - do nothing. */
    }
  }
  pthread_mutex_unlock(&__nc_thread_management_lock);
  return retval;
}

int pthread_kill(pthread_t thread_id,
                 int sig) {
  /* This function is currently unimplemented. */
  return ENOSYS;
}

pthread_t pthread_self(void) {
  /* Get the tdb pointer from gs and use it to return the thread handle. */
  nc_thread_descriptor_t *tdb = nc_get_tdb();
  return tdb->basic_data;
}

int pthread_equal(pthread_t thread1, pthread_t thread2) {
  return (thread1 == thread2);
}

int pthread_setschedprio(pthread_t thread_id, int prio) {
  if (thread_id != pthread_self()) {
    /*
     * We can only support changing our own priority.
     */
    return EPERM;
  }
  return __libnacl_irt_thread.thread_nice(prio);
}

int pthread_attr_init(pthread_attr_t *attr) {
  if (NULL == attr) {
    return EINVAL;
  }
  attr->joinable = PTHREAD_CREATE_JOINABLE;
  attr->stacksize = PTHREAD_STACK_DEFAULT;
  return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
  if (NULL == attr) {
    return EINVAL;
  }
  /* Nothing to destroy. */
  return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr,
                                int detachstate) {
  if (NULL == attr) {
    return EINVAL;
  }
  attr->joinable = detachstate;
  return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr,
                                int *detachstate) {
  if (NULL == attr) {
    return EINVAL;
  }
  return attr->joinable;
}

int pthread_attr_setscope(pthread_attr_t *attr, int scope) {
  if (scope == PTHREAD_SCOPE_PROCESS) {
    return ENOTSUP;
  }
  if (scope != PTHREAD_SCOPE_SYSTEM) {
    return EINVAL;
  }
  return 0;
}

int pthread_attr_getscope(const pthread_attr_t *attr, int *scope) {
  *scope = PTHREAD_SCOPE_SYSTEM;
  return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr,
                              size_t stacksize) {
  if (NULL == attr) {
    return EINVAL;
  }
  if (PTHREAD_STACK_MIN < stacksize) {
    attr->stacksize = stacksize;
  } else {
    attr->stacksize = PTHREAD_STACK_MIN;
  }
  return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr,
                              size_t *stacksize) {
  if (NULL == attr) {
    return EINVAL;
  }
  *stacksize = attr->stacksize;
  return 0;
}

void __local_lock_init(_LOCK_T *lock);
void __local_lock_init_recursive(_LOCK_T *lock);
void __local_lock_close(_LOCK_T *lock);
void __local_lock_close_recursive(_LOCK_T *lock);
void __local_lock_acquire(_LOCK_T *lock);
void __local_lock_acquire_recursive(_LOCK_T *lock);
int __local_lock_try_acquire(_LOCK_T *lock);
int __local_lock_try_acquire_recursive(_LOCK_T *lock);
void __local_lock_release(_LOCK_T *lock);
void __local_lock_release_recursive(_LOCK_T *lock);

void __local_lock_init(_LOCK_T *lock) {
  if (lock != NULL) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_FAST_NP);
    pthread_mutex_init((pthread_mutex_t*)lock, &attr);
  }
}

void __local_lock_init_recursive(_LOCK_T *lock) {
  if (lock != NULL) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init((pthread_mutex_t*)lock, &attr);
  }
}

void __local_lock_close(_LOCK_T *lock) {
  if (lock != NULL) {
    pthread_mutex_destroy((pthread_mutex_t*)lock);
  }
}

void __local_lock_close_recursive(_LOCK_T *lock) {
  __local_lock_close(lock);
}

void __local_lock_acquire(_LOCK_T *lock) {
  if (!__nc_thread_initialized) {
    /*
     * pthread library is not initialized yet - there is only one thread.
     * Calling pthread_mutex_lock will cause an access violation because it
     * will attempt to access the TDB which is not initialized yet.
     */
    return;
  }
  if (lock != NULL) {
    pthread_mutex_lock((pthread_mutex_t*)lock);
  }
}

void __local_lock_acquire_recursive(_LOCK_T *lock) {
  __local_lock_acquire(lock);
}

int __local_lock_try_acquire(_LOCK_T *lock) {
  if (!__nc_thread_initialized) {
    /*
     * pthread library is not initialized yet - there is only one thread.
     * Calling pthread_mutex_lock will cause an access violation because it
     * will attempt to access the TDB which is not initialized yet.
     */
    return 0;
  }

  if (lock != NULL) {
    return pthread_mutex_trylock((pthread_mutex_t*)lock);
  } else {
    return EINVAL;
  }
}

int __local_lock_try_acquire_recursive(_LOCK_T *lock) {
  return __local_lock_try_acquire(lock);
}

void __local_lock_release(_LOCK_T *lock) {
  if (!__nc_thread_initialized) {
    /*
     * pthread library is not initialized yet - there is only one thread.
     * Calling pthread_mutex_lock will cause an access violation because it
     * will attempt to access the TDB which is not initialized yet
     * NOTE: there is no race condition here because the value of the counter
     * cannot change while the lock is held - the startup process is
     * single-threaded.
     */
    return;
  }

  if (lock != NULL) {
    pthread_mutex_unlock((pthread_mutex_t*)lock);
  }
}

void __local_lock_release_recursive(_LOCK_T *lock) {
  __local_lock_release(lock);
}

/*
 * We include this directly in this file rather than compiling it
 * separately because there is some code (e.g. libstdc++) that uses weak
 * references to all pthread functions, but conditionalizes its calls only
 * on one symbol.  So if these functions are in another file in a library
 * archive, they might not be linked in by static linking.
 */
/* @IGNORE_LINES_FOR_CODE_HYGIENE[1] */
#include "native_client/src/untrusted/pthread/nc_tsd.c"
