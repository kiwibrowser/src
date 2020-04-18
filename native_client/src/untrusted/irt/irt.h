/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

struct timeval;
struct timespec;
struct dirent;

struct NaClExceptionContext;
struct NaClMemMappingInfo;

typedef int64_t nacl_irt_off_t;
typedef int32_t nacl_irt_clock_t;
typedef uint32_t nacl_irt_clockid_t;
typedef uintptr_t nacl_irt_tid_t;
#define NACL_IRT_MAIN_THREAD_TID 0

/*
 * Under newlib, the 'struct stat' defined in <sys/stat.h> matches the
 * layout of 'struct nacl_abi_stat', so the standard type 'struct stat'
 * is used in IRT function signatures.  Under glibc, the 'struct stat'
 * defined by the C library differs and translation between the C
 * library format and the IRT format is required.
 */
#if defined(__native_client__) && defined(__GLIBC__)
typedef struct nacl_abi_stat nacl_irt_stat_t;
#else
typedef struct stat nacl_irt_stat_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * The only interface exposed directly to user code is a single function
 * of this type.  It is passed via the AT_SYSINFO field of the ELF
 * auxiliary vector on the stack at program startup.  The interfaces
 * below are accessed by calling this function with the appropriate
 * interface identifier.
 *
 * This function returns the number of bytes filled in at TABLE, which
 * is never larger than TABLESIZE.  If the interface identifier is not
 * recognized or TABLESIZE is too small, it returns zero.
 *
 * This function writes each function pointer to TABLE atomically.
 *
 * The interface of the query function avoids passing any data pointers
 * back from the IRT to user code.  Only code pointers are passed back.
 * It is an opaque implementation detail (that may change) whether those
 * point to normal untrusted code in the user address space, or whether
 * they point to special trampoline addresses supplied by trusted code.
 */
typedef size_t (*TYPE_nacl_irt_query)(const char *interface_ident,
                                      void *table, size_t tablesize);

/*
 * C libraries expose this function to reach the interface query interface.
 * If there is no IRT hook available at all, it returns zero.
 * Otherwise it behaves as described above for TYPE_nacl_irt_query.
 */
size_t nacl_interface_query(const char *interface_ident,
                            void *table, size_t tablesize);

/*
 * General notes about IRT interfaces:
 *
 * All functions in IRT vectors return an int, which is zero for success
 * or a (positive) errno code for errors.  Any values are delivered via
 * result parameters.  The only exceptions exit/thread_exit, which can
 * never return, and tls_get, which can never fail.
 *
 * Some of the IRT interfaces below are disabled under PNaCl because
 * they are deprecated or not portable.  The list of IRT interfaces
 * that are allowed under PNaCl can be found in the Chromium repo in
 * ppapi/native_client/src/untrusted/pnacl_irt_shim/shim_ppapi.c.
 *
 * Interfaces with "-dev" in the query string are not
 * permanently-supported stable interfaces.  They might be removed in
 * future versions of Chromium.
 */

#define NACL_IRT_BASIC_v0_1     "nacl-irt-basic-0.1"
struct nacl_irt_basic {
  void (*exit)(int status);
  int (*gettod)(struct timeval *tv);
  int (*clock)(nacl_irt_clock_t *ticks);
  int (*nanosleep)(const struct timespec *req, struct timespec *rem);
  int (*sched_yield)(void);
  int (*sysconf)(int name, int *value);
};

/*
 * "irt-fdio" provides IO operations on file descriptors (FDs).  There
 * are three cases where this interface is useful under Chromium:
 *
 * 1) With the read-only FDs returned by open_resource().  This use
 *    case does not apply to PNaCl, where open_resource() is disabled.
 * 2) write() on stdout or stderr can be useful for writing debugging
 *    output, but it does not produce any effects observable to a web
 *    app.  In Chromium, whether write() returns an error is not
 *    defined (see
 *    https://code.google.com/p/nativeclient/issues/detail?id=3529).
 * 3) With FDs returned by open().  In Chromium, this only applies when
 *    NACL_DANGEROUS_ENABLE_FILE_ACCESS is set, which enables an
 *    unsafe debugging mode.
 *
 * There are two query strings for this interface.  Under PNaCl, this
 * interface is only available via the "-dev" query string, because
 * the only uses cases for it under PNaCl are for debugging -- (2) and
 * (3).  However, as with all "-dev" interfaces, the "-dev" variant
 * might be removed in future.
 */
#define NACL_IRT_FDIO_v0_1      "nacl-irt-fdio-0.1"
#define NACL_IRT_DEV_FDIO_v0_1  "nacl-irt-dev-fdio-0.1"
struct nacl_irt_fdio {
  int (*close)(int fd);
  int (*dup)(int fd, int *newfd);
  int (*dup2)(int fd, int newfd);
  int (*read)(int fd, void *buf, size_t count, size_t *nread);
  int (*write)(int fd, const void *buf, size_t count, size_t *nwrote);
  int (*seek)(int fd, nacl_irt_off_t offset, int whence,
              nacl_irt_off_t *new_offset);
  int (*fstat)(int fd, nacl_irt_stat_t *);
  int (*getdents)(int fd, struct dirent *, size_t count, size_t *nread);
};

/*
 * The "irt-filename" interface provides filename-based filesystem
 * operations.  In Chromium, this is only useful when
 * NACL_DANGEROUS_ENABLE_FILE_ACCESS is set, which enables an unsafe
 * debugging mode.
 *
 * Under PNaCl, this interface is not available. This interface is made
 * available to non-PNaCl NaCl apps only for compatibility, because
 * existing nexes abort on startup if "irt-filename" is not available.
 */
#define NACL_IRT_FILENAME_v0_1      "nacl-irt-filename-0.1"
struct nacl_irt_filename {
  int (*open)(const char *pathname, int oflag, mode_t cmode, int *newfd);
  int (*stat)(const char *pathname, nacl_irt_stat_t *);
};

/*
 * This old version of irt-memory is disabled under PNaCl because it
 * contains sysbrk() (see
 * https://code.google.com/p/nativeclient/issues/detail?id=3542).
 */
#define NACL_IRT_MEMORY_v0_1    "nacl-irt-memory-0.1"
struct nacl_irt_memory_v0_1 {
  /*
   * sysbrk() allocates memory from the "brk" heap.  This function is
   * deprecated; new programs should use mmap() instead.
   *
   * If |*newbrk| is NULL, sysbrk() sets |*newbrk| to the current
   * break pointer and returns 0.
   *
   * If |*newbrk| is non-NULL and greater than the current break
   * pointer, sysbrk() tries to allocate this memory.  If the
   * allocation fails, it returns ENOMEM.  Otherwise, sysbrk():
   *  * ensures the memory between the break pointer and |*newbrk| is
   *    readable and writable, and zeroes it;
   *  * sets the current break pointer to |*newbrk|; and
   *  * returns 0 to indicate success.
   *
   * If |*newbrk| is non-NULL and less than the current break pointer,
   * sysbrk() deallocates this memory.  sysbrk() sets the break
   * pointer to |*newbrk| and returns 0.  If |*newbrk| is less than
   * the process's initial break pointer, the behaviour is undefined.
   */
  int (*sysbrk)(void **newbrk);
  /* Note: this version of mmap silently ignores PROT_EXEC bit.  */
  int (*mmap)(void **addr, size_t len, int prot, int flags, int fd,
              nacl_irt_off_t off);
  int (*munmap)(void *addr, size_t len);
};

/* This old version of irt-memory is also disabled under PNaCl. */
#define NACL_IRT_MEMORY_v0_2    "nacl-irt-memory-0.2"
struct nacl_irt_memory_v0_2 {
  int (*sysbrk)(void **newbrk);
  int (*mmap)(void **addr, size_t len, int prot, int flags, int fd,
              nacl_irt_off_t off);
  int (*munmap)(void *addr, size_t len);
  int (*mprotect)(void *addr, size_t len, int prot);
};

#define NACL_IRT_MEMORY_v0_3    "nacl-irt-memory-0.3"
struct nacl_irt_memory {
  int (*mmap)(void **addr, size_t len, int prot, int flags, int fd,
              nacl_irt_off_t off);
  int (*munmap)(void *addr, size_t len);
  int (*mprotect)(void *addr, size_t len, int prot);
};

/*
 * This interface is disabled under PNaCl because it allows
 * dynamically loading architecture-specific native code, which is not
 * portable.
 */
#define NACL_IRT_DYNCODE_v0_1   "nacl-irt-dyncode-0.1"
struct nacl_irt_dyncode {
  int (*dyncode_create)(void *dest, const void *src, size_t size);
  int (*dyncode_modify)(void *dest, const void *src, size_t size);
  int (*dyncode_delete)(void *dest, size_t size);
};

#define NACL_IRT_THREAD_v0_1   "nacl-irt-thread-0.1"
struct nacl_irt_thread {
  /*
   * thread_create() starts a new thread which calls start_func().
   *
   * In the new thread, tls_get() (from nacl_irt_tls) will return
   * |thread_ptr|.  start_func() is called with no arguments, so
   * |thread_ptr| is the only way to pass parameters to the new
   * thread.
   *
   * |stack| is a pointer to the top of the stack for the new thread.
   * Note that this assumes the stack grows downwards.
   *
   * |stack| does not need to be aligned.  thread_func() will be
   * called with a stack pointer aligned appropriately for the
   * architecture's ABI.  (However, prior to r9299, from July 2012,
   * |stack| did need to be aligned for thread_func() to be called
   * with an appropriately aligned stack pointer.)
   *
   * The exact stack pointer that thread_func() is called with may be
   * less than |stack|, and the system may write data to addresses
   * below |stack| before calling start_func(), so user code may not
   * use the stack as a way to pass parameters to start_func().
   *
   * If start_func() returns in the new thread, the behaviour is
   * undefined.
   */
  int (*thread_create)(void (*start_func)(void), void *stack, void *thread_ptr);
  /*
   * thread_exit() terminates the current thread.
   *
   * If |stack_flag| is non-NULL, thread_exit() will write 0 to
   * |*stack_flag|.  This is intended to be used by a threading
   * library to determine when the thread's stack can be deallocated
   * or reused.  The system will not read or write the thread's stack
   * after writing 0 to |*stack_flag|.
   */
  void (*thread_exit)(int32_t *stack_flag);
  int (*thread_nice)(const int nice);
};

/*
 * This interface is only available on Non-SFI Mode.
 */
#define NACL_IRT_THREAD_v0_2   "nacl-irt-thread-0.2"
struct nacl_irt_thread_v0_2 {
  /*
   * This interface is the same as nacl_irt_thread_v0_1, with the additional
   * feature that it assigns the thread ID of the child thread into
   * |child_tid| before the child thread starts and before execution control
   * returns to the parent.
   */
  int (*thread_create)(void (*start_func)(void), void *stack, void *thread_ptr,
                       nacl_irt_tid_t *child_tid);
  void (*thread_exit)(int32_t *stack_flag);
  int (*thread_nice)(const int nice);
};

/*
 * The irt_futex interface is based on Linux's futex() system call.
 *
 * irt_futex provides process-private futexes, so futex wait queues are
 * associated with numeric virtual addresses only.  This is equivalent to
 * Linux's FUTEX_PRIVATE_FLAG.  If a page is mmap()'d twice, futex_wake()
 * on one mapping will *not* wake a thread that is waiting on the other
 * mapping with futex_wait_abs() -- futex wait queues are not associated
 * with pages' identities.
 */
#define NACL_IRT_FUTEX_v0_1        "nacl-irt-futex-0.1"
struct nacl_irt_futex {
  /*
   * If |*addr| still contains |value|, futex_wait_abs() waits to be
   * woken up by a futex_wake(addr,...) call from another thread;
   * otherwise, it immediately returns EAGAIN (which is the same as
   * EWOULDBLOCK).  If woken by another thread, it returns 0.  If
   * |abstime| is non-NULL and the time specified by |*abstime|
   * passes, this returns ETIMEDOUT.
   *
   * Note that this differs from Linux's FUTEX_WAIT in that it takes an
   * absolute time value (relative to the Unix epoch) rather than a
   * relative time duration.
   */
  int (*futex_wait_abs)(volatile int *addr, int value,
                        const struct timespec *abstime);
  /*
   * futex_wake() wakes up threads that are waiting on |addr| using
   * futex_wait().  |nwake| is the maximum number of threads that will
   * be woken up.  The number of threads that were woken is returned
   * in |*count|.
   */
  int (*futex_wake)(volatile int *addr, int nwake, int *count);
};

/*
 * "irt-mutex" is deprecated and is disabled under PNaCl (see
 * https://code.google.com/p/nativeclient/issues/detail?id=3484).
 * nacl-newlib's libpthread no longer uses it.  Note, however, that
 * nacl-glibc's futex_emulation.c still uses it.
 */
#define NACL_IRT_MUTEX_v0_1        "nacl-irt-mutex-0.1"
struct nacl_irt_mutex {
  int (*mutex_create)(int *mutex_handle);
  int (*mutex_destroy)(int mutex_handle);
  int (*mutex_lock)(int mutex_handle);
  int (*mutex_unlock)(int mutex_handle);
  int (*mutex_trylock)(int mutex_handle);
};

/*
 * "irt-cond" is deprecated and is disabled under PNaCl (see
 * https://code.google.com/p/nativeclient/issues/detail?id=3484).
 * nacl-newlib's libpthread no longer uses it.  Note, however, that
 * nacl-glibc's futex_emulation.c still uses it.
 */
#define NACL_IRT_COND_v0_1      "nacl-irt-cond-0.1"
struct nacl_irt_cond {
  int (*cond_create)(int *cond_handle);
  int (*cond_destroy)(int cond_handle);
  int (*cond_signal)(int cond_handle);
  int (*cond_broadcast)(int cond_handle);
  int (*cond_wait)(int cond_handle, int mutex_handle);
  int (*cond_timed_wait_abs)(int cond_handle, int mutex_handle,
                             const struct timespec *abstime);
};

/*
 * The "irt-sem" interface provides semaphores.  This interface is
 * deprecated and is disabled under PNaCl (see
 * https://code.google.com/p/nativeclient/issues/detail?id=3484).  New
 * versions of nacl-newlib's libpthread no longer use it, and
 * nacl-glibc has never used it.  They implement semaphores using
 * futexes instead.
 */
#define NACL_IRT_SEM_v0_1       "nacl-irt-sem-0.1"
struct nacl_irt_sem {
  int (*sem_create)(int *sem_handle, int32_t value);
  int (*sem_destroy)(int sem_handle);
  int (*sem_post)(int sem_handle);
  int (*sem_wait)(int sem_handle);
};

#define NACL_IRT_TLS_v0_1       "nacl-irt-tls-0.1"
struct nacl_irt_tls {
  int (*tls_init)(void *thread_ptr);
  void *(*tls_get)(void);
};

/*
 * The "irt-blockhook" interface is disabled under PNaCl because it
 * does not have a known-portable use case (see
 * https://code.google.com/p/nativeclient/issues/detail?id=3539).
 */
#define NACL_IRT_BLOCKHOOK_v0_1 "nacl-irt-blockhook-0.1"
struct nacl_irt_blockhook {
  int (*register_block_hooks)(void (*pre)(void), void (*post)(void));
};

/*
 * In Chromium, open_resource() opens a file listed in the NaCl
 * manifest file (NMF).  It returns a read-only file descriptor.
 *
 * This interface is disabled under PNaCl because it was provided
 * primarily for use by nacl-glibc's dynamic linker, which is not
 * supported under PNaCl.  Also, open_resource() returns a file
 * descriptor, but it is the only interface in NaCl to do so inside
 * Chromium.  This is inconsistent with PPAPI, which does not expose
 * file descriptors (except in private/dev interfaces).  See:
 * https://code.google.com/p/nativeclient/issues/detail?id=3574
 */
#define NACL_IRT_RESOURCE_OPEN_v0_1 "nacl-irt-resource-open-0.1"
struct nacl_irt_resource_open {
  int (*open_resource)(const char *file, int *fd);
};

#define NACL_IRT_RANDOM_v0_1 "nacl-irt-random-0.1"
struct nacl_irt_random {
  int (*get_random_bytes)(void *buf, size_t count, size_t *nread);
};

#define NACL_IRT_CLOCK_v0_1 "nacl-irt-clock_get-0.1"
struct nacl_irt_clock {
  /*
   * clock_getres finds the resolution of the given clock; its pointer
   * argument may be NULL, in which case the call does nothing but check
   * the validity of the clock_id argument (returning EINVAL or 0).
   */
  int (*clock_getres)(nacl_irt_clockid_t clock_id, struct timespec *res);
  /*
   * clock_gettime samples the current value of the given clock;
   * its pointer argument may not be NULL.
   */
  int (*clock_gettime)(nacl_irt_clockid_t clock_id, struct timespec *tp);
};

/*
 * This interface is disabled under PNaCl because it exposes
 * non-portable, architecture-specific register state.
 */
#define NACL_IRT_EXCEPTION_HANDLING_v0_1 \
  "nacl-irt-exception-handling-0.1"
typedef void (*NaClExceptionHandler)(struct NaClExceptionContext *context);
struct nacl_irt_exception_handling {
  int (*exception_handler)(NaClExceptionHandler handler,
                           NaClExceptionHandler *old_handler);
  int (*exception_stack)(void *stack, size_t size);
  int (*exception_clear_flag)(void);
};

#define NACL_IRT_CODE_DATA_ALLOC_v0_1 "nacl-irt-code-data-alloc-0.1"
struct nacl_irt_code_data_alloc {
  /*
   * Atomically allocate a code segment along with an associated data
   * segment with a specified offset. The caller can then call mmap or
   * dyncode_create for each of the segments starting with the value returned
   * by |*begin|.
   *
   * |hint| is a desired address where the code should begin. If |hint| is 0
   * this value is ignored and the next available combination of code and data
   * segment will be allocated. The hint must be page aligned and is ignored if
   * the hint location is already reserved.
   *
   * |code_size| is the size for the code segment, it should be non-zero and
   * page size aligned.
   *
   * |data_offset| is the offset between the start of the code segment and the
   * start of the data segment. If |data_size| is zero, this value is ignored.
   *
   * |data_size| is the size of the data segment, if no data segment is required
   * this value can be 0. If the function is successful and |data_size| is
   * non-zero, the address space for the data segment is reserved and will not
   * be chosen by allocate_code_data calls or mmap calls without MAP_FIXED. The
   * data segment will be located at the address |*begin| + |data_offset|.
   *
   * |*begin| is where the code segment is reserved so future calls to
   * allocate_code_data will not overlap address segments.
   */
  int (*allocate_code_data)(uintptr_t hint, size_t code_size,
                            uintptr_t data_offset,
                            size_t data_size,
                            uintptr_t *begin);
};

/*
 * This interface is only available on ARM, only for Non-SFI Mode.
 */
#define NACL_IRT_ICACHE_v0_1 "nacl-irt-icache-0.1"
struct nacl_irt_icache {
  /*
   * clear_cache() makes instruction cache and data cache for the address
   * range from |addr| to |(intptr_t)addr + size| (exclusive) coherent.
   */
  int (*clear_cache)(void *addr, size_t size);
};

/*
 * This interface is only available on Non-SFI Mode.
 *
 * For more information, consult documentation/nonsfi_mode_async_signals.txt.
 */

typedef void (*NaClIrtAsyncSignalHandler)(
    struct NaClExceptionContext *context);

#define NACL_IRT_ASYNC_SIGNAL_HANDLING_v0_1 "nacl-irt-async-signal-handling-0.1"
struct nacl_irt_async_signal_handling {
  /*
   * NaCl applications can register a single, global async signal handler that
   * will be invoked on the thread that receives the signal.  When
   * send_async_signal is called, |handler| is invoked on the thread specified
   * by |tid| on the thread's stack and without modifying the underlying signal
   * mask (as opposed to the POSIX behavior), so it is possible for |handler| to
   * be invoked recursively (if a signal is sent from |handler|) and
   * concurrently (if two threads send signals at the same time).
   *
   * If the thread was executing an IRT function, the suspended thread will
   * continue it after |handler| finishes.  In other words, no IRT function will
   * be aborted.
   *
   * You can safely use only async-signal-safe operations in |handler|. The
   * following NaCl IRT functions are async-signal-safe:
   *
   * - tls_get
   * - futex_wait_abs
   * - futex_wake
   * - send_async_signal
   *
   * This function in particular is not async-signal-safe and must not be called
   * from signal handlers.
   */
  int (*set_async_signal_handler)(NaClIrtAsyncSignalHandler handler);
  /*
   * Asynchronously delivers a signal to the thread identified by |tid| within
   * the same process as the caller.  |tid| should be a valid thread identifier
   * obtained when creating a thread (through the |child_tid| parameter to
   * create_thread()) and the thread must still be alive.  The constant
   * NACL_IRT_MAIN_THREAD_TID can be used to refer to the main thread.
   *
   * As opposed to POSIX, NaCl only provides a way to send a single signal.
   * Delivery of different kinds of signals to be POSIX-compliant can be
   * implemented in userspace.
   *
   * This IRT function returns 0 on success and may return ESRCH or EINVAL on
   * failure.  When |tid| refers to a thread that has already exited, it is
   * possible that the ID has been recycled for a new thread, so it might also
   * succeed and send the signal to the new thread.
   */
  int (*send_async_signal)(nacl_irt_tid_t tid);
};

#if defined(__cplusplus)
}
#endif

#endif /* NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_H */
