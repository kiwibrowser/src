/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>

#include "native_client/src/nonsfi/linux/linux_pthread_private.h"
#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"
#include "native_client/src/nonsfi/linux/linux_syscall_structs.h"
#include "native_client/src/nonsfi/linux/linux_syscall_wrappers.h"
#include "native_client/src/nonsfi/linux/linux_sys_private.h"
#include "native_client/src/public/linux_syscalls/sched.h"
#include "native_client/src/public/linux_syscalls/sys/syscall.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/nacl_thread.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"

/* Convert a return value of a Linux syscall to the one of an IRT call. */
static uint32_t irt_return_call(uintptr_t result) {
  if (linux_is_error_result(result))
    return -result;
  return 0;
}

static int nacl_irt_thread_create_v0_2(void (*start_func)(void), void *stack,
                                       void *thread_ptr,
                                       nacl_irt_tid_t *child_tid) {
  /*
   * We do not use CLONE_CHILD_CLEARTID as we do not want any
   * non-private futex signaling. Also, NaCl ABI does not require us
   * to signal the futex on stack_flag.
   */
  int flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
               CLONE_THREAD | CLONE_SYSVSEM | CLONE_SETTLS |
               CLONE_PARENT_SETTID);
  /*
   * In order to avoid allowing clone with and without CLONE_PARENT_SETTID, if
   * |child_tid| is NULL, we provide a valid pointer whose value will be
   * ignored.
   */
  nacl_irt_tid_t ignored;
  void *ptid = (child_tid != NULL) ? child_tid : &ignored;

  /*
   * linux_clone_wrapper expects start_func's type is "int (*)(void *)".
   * Although |start_func| has type "void (*)(void)", the type mismatching
   * will not cause a problem. Passing a dummy |arg| (= 0) does nothing there.
   * Also, start_func will never return.
   */
  return irt_return_call(linux_clone_wrapper(
      (uintptr_t) start_func, /* arg */ 0, flags, stack,
      ptid, thread_ptr, /* ctid */ NULL));
}

static int nacl_irt_thread_create(void (*start_func)(void), void *stack,
                                  void *thread_ptr) {
  nacl_irt_tid_t child_tid;
  return nacl_irt_thread_create_v0_2(start_func, stack, thread_ptr, &child_tid);
}

static void nacl_irt_thread_exit(int32_t *stack_flag) {
  /*
   * We fill zero to stack_flag by ourselves instead of relying
   * on CLONE_CHILD_CLEARTID. We do everything in the following inline
   * assembly because we need to make sure we will never touch stack.
   *
   * We will set the stack pointer to zero at the beginning of the
   * assembly code just in case an async signal arrives after setting
   * *stack_flag=0 but before calling the syscall, so that any signal
   * handler crashes rather than running on a stack that has been
   * reallocated to another thread.
   */
#if defined(__i386__)
  __asm__ __volatile__("mov $0, %%esp\n"
                       "movl $0, (%[stack_flag])\n"
                       "int $0x80\n"
                       "hlt\n"
                       :: [stack_flag]"r"(stack_flag), "a"(__NR_exit));
#elif defined(__arm__)
  __asm__ __volatile__("mov sp, #0\n"
                       "mov r7, #0\n"
                       "str r7, [%[stack_flag]]\n"
                       "mov r7, %[sysno]\n"
                       "svc #0\n"
                       "bkpt #0\n"
                       :: [stack_flag]"r"(stack_flag), [sysno]"i"(__NR_exit)
                       : "r7");
#else
# error Unsupported architecture
#endif
}

static int nacl_irt_thread_nice(const int nice) {
  return 0;
}

static struct nc_combined_tdb *get_irt_tdb(void *thread_ptr) {
  struct nc_combined_tdb *tdb = (void *) ((uintptr_t) thread_ptr +
                                          __nacl_tp_tdb_offset(sizeof(*tdb)));
  return tdb;
}

/*
 * This is the real first entry point for new threads.
 * Based on code from src/untrusted/irt/irt_thread.c
 */
static void irt_start_thread() {
  struct nc_combined_tdb *tdb = get_irt_tdb(__nacl_read_tp());

  /*
   * Fetch the user's start routine.
   */
  void *(*user_start)(void *) = tdb->tdb.start_func;

  /*
   * Now do per-thread initialization for the IRT-private C library state.
   */
  __newlib_thread_init();

  /*
   * Finally, run the user code.
   */
  (*user_start)(tdb->tdb.state);

  /*
   * That should never return.  Crash hard if it does.
   */
  __builtin_trap();
}

/*
 * Creates a thread and initializes the IRT-private TLS area.
 * Based on code from src/untrusted/irt/irt_thread.c
 */
int nacl_user_thread_create(void *(*start_func)(void *), void *stack,
                            void *thread_ptr, nacl_irt_tid_t *child_tid) {
  struct nc_combined_tdb *tdb;

  /*
   * Before we start the thread, allocate the IRT-private TLS area for it.
   */
  size_t combined_size = __nacl_tls_combined_size(sizeof(*tdb));
  void *combined_area = malloc(combined_size);
  if (combined_area == NULL)
    return EAGAIN;

 /*
  * Note that __nacl_tls_initialize_memory() is not reversible,
  * because it takes a pointer that need not be aligned and can
  * return a pointer that is aligned.  In order to
  * free(combined_area) later, we must save the value of
  * combined_area.
  */
  void *irt_tp = __nacl_tls_initialize_memory(combined_area, sizeof(*tdb));
  tdb = get_irt_tdb(irt_tp);
  __nc_initialize_unjoinable_thread(tdb);
  tdb->tdb.irt_thread_data = combined_area;
  tdb->tdb.start_func = start_func;
  tdb->tdb.state = thread_ptr;

  return nacl_irt_thread_create_v0_2(irt_start_thread, stack, irt_tp,
                                     child_tid);
}

/*
 * Destroys a thread created by nacl_user_thread_create.
 * Based on code from src/untrusted/irt/irt_thread.c
 */
void nacl_user_thread_exit(int32_t *stack_flag) {
  struct nc_combined_tdb *tdb = get_irt_tdb(__nacl_read_tp());

  __nc_tsd_exit();

  /*
   * Sanity check: Check that this function was not called on a thread
   * created by the IRT's internal pthread_create().  For such
   * threads, irt_thread_data == NULL.
   */
  assert(tdb->tdb.irt_thread_data != NULL);

  free(tdb->tdb.irt_thread_data);

  nacl_irt_thread_exit(stack_flag);
}

void __nc_initialize_interfaces(void) {
  const struct nacl_irt_thread init = {
    nacl_irt_thread_create,
    nacl_irt_thread_exit,
    nacl_irt_thread_nice,
  };
  __libnacl_irt_thread = init;
}
