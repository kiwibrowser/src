/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 *
 * This loader can only process NaCl object files as produced using
 * the NaCl toolchain.  Other ELF files will be rejected.
 *
 * The primary function, NaClAppLoadFile, parses an ELF file,
 * allocates memory, loads the relocatable image from the ELF file
 * into memory, and performs relocation.  NaClAppRun runs the
 * resultant program.
 *
 * This loader is written in C so that it can be used by C-only as
 * well as C++ applications.  Other languages should also be able to
 * use their foreign-function interfaces to invoke C code.
 *
 * This loader must be part of the NaCl TCB, since it directly handles
 * externally supplied input (the ELF file).  Any security
 * vulnerabilities in handling the ELF image, e.g., buffer or integer
 * overflows, can put the application at risk.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_H_ 1

#include "native_client/src/include/atomic_ops.h"
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/elf.h"

#include "native_client/src/public/imc_types.h"
#include "native_client/src/public/nacl_app.h"

#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_threads.h"

#include "native_client/src/trusted/interval_multiset/nacl_interval_multiset.h"
#include "native_client/src/trusted/interval_multiset/nacl_interval_range_tree.h"

#include "native_client/src/trusted/service_runtime/dyn_array.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"
#include "native_client/src/trusted/service_runtime/nacl_resource.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_handlers.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_mem.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"
#include "native_client/src/trusted/service_runtime/sys_futex.h"

#include "native_client/src/trusted/validator/ncvalidate.h"

EXTERN_C_BEGIN

#define NACL_DEFAULT_STACK_MAX  (16 << 20)  /* main thread stack */

struct NaClAppThread;
struct NaClDesc;  /* see native_client/src/trusted/desc/nacl_desc_base.h */
struct NaClDynamicRegion;
struct NaClSignalContext;
struct NaClValidationCache;
struct NaClValidationMetadata;

struct NaClDebugCallbacks {
  void (*thread_create_hook)(struct NaClAppThread *natp);
  void (*thread_exit_hook)(struct NaClAppThread *natp);
  void (*process_exit_hook)(void);
};

#if NACL_WINDOWS
enum NaClDebugExceptionHandlerState {
  NACL_DEBUG_EXCEPTION_HANDLER_NOT_STARTED,
  NACL_DEBUG_EXCEPTION_HANDLER_STARTED,
  NACL_DEBUG_EXCEPTION_HANDLER_FAILED
};
/*
 * Callback function used to request that an exception handler be
 * attached using the Windows debug API.  See sel_main_chrome.h.
 */
typedef int (*NaClAttachDebugExceptionHandlerFunc)(const void *info,
                                                   size_t size);
#endif

struct NaClSpringboardInfo {
  /* These are addresses in untrusted address space (relative to mem_start). */
  uint32_t start_addr;
  uint32_t end_addr;
};

struct NaClApp {
  /*
   * public, user settable prior to app start.
   */
  uint8_t                   addr_bits;
  uintptr_t                 stack_size;
  uint32_t                  initial_nexe_max_code_bytes;
  /*
   * stack_size is the maximum size of the (main) stack.  The stack
   * memory is eager allocated (mapped in w/o MAP_NORESERVE) so
   * there must be enough swap space; page table entries are not
   * populated (no MAP_POPULATE), so actual accesses will likely
   * incur page faults.
   */

  /*
   * Determined at load time; OS-determined.
   * Read-only after load, so accesses do not require locking.
   */
  uintptr_t                 mem_start;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  uintptr_t                 pcrel_thunk;
  uintptr_t                 pcrel_thunk_end;
#endif
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  uintptr_t                 nacl_syscall_addr;
  uintptr_t                 get_tls_fast_path1_addr;
  uintptr_t                 get_tls_fast_path2_addr;
#endif

  /* only used for ET_EXEC:  for CS restriction */
  uintptr_t                 static_text_end;
  /*
   * relative to mem_start; ro after app starts. memsz from phdr
   */

  /*
   * The dynamic code area follows the static code area.  These fields
   * are both set to static_text_end if the dynamic code area has zero
   * size.
   */
  uintptr_t                 dynamic_text_start;
  uintptr_t                 dynamic_text_end;

  /*
   * rodata_start and data_start may be 0 if these segments are not
   * present in the executable.
   */
  uintptr_t                 rodata_start;  /* initialized data, ro */
  uintptr_t                 data_start;    /* initialized data/bss, rw */
  /*
   * Various region sizes must be a multiple of NACL_MAP_PAGESIZE
   * before the NaCl app can run.  The sizes from the ELF file
   * (p_filesz field) might not be -- that would waste space for
   * padding -- and while we could use p_memsz to specify padding, but
   * we will record the virtual addresses of the start of the segments
   * and figure out the gap between the p_vaddr + p_filesz of one
   * segment and p_vaddr of the next to determine padding.
   */

  uintptr_t                 data_end;
  /* see break_addr below */

  /*
   * initial_entry_pt is the first address in untrusted code to jump
   * to.  When using the IRT (integrated runtime), this is provided by
   * the IRT library, and user_entry_pt is the entry point in the user
   * executable.  Otherwise, initial_entry_pt is in the user
   * executable and user_entry_pt is zero.
   */
  uintptr_t                 initial_entry_pt;
  uintptr_t                 user_entry_pt;

  /*
   * bundle_size is the bundle alignment boundary for validation (16
   * or 32), so int is okay.  This value must be a power of 2.
   */
  int                       bundle_size;

  /* common to both ELF executables and relocatable load images */

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  /* Addresses of trusted springboard code for switching to untrusted code. */
  struct NaClSpringboardInfo syscall_return_springboard;
  struct NaClSpringboardInfo all_regs_springboard;
#endif

  /*
   * The main NaCl executable may already be validated during ELF
   * loading, where after a validation cache hit the code gets mmapped
   * into memory if the file descriptor is "blessed" as referring to a
   * file which the embedding environment guarantees to be effectively
   * immutable.  If it did not validate or the file descriptor is not
   * blessed, then the code is read into memory, and we will validate
   * it later in the code path, in NaClAppLoadFileAslr.
   */
  int main_exe_prevalidated;  /* bool */

  struct NaClMutex          mu;
  struct NaClCondVar        cv;

#if NACL_WINDOWS
  /*
   * invariant: !(vm_hole_may_exist && threads_launching != 0).
   * vm_hole_may_exist is set while mmap/munmap manipulates the memory
   * map, and threads_launching is set while a thread is launching
   * (and a trusted thread stack is being allocated).
   *
   * strictly speaking, vm_hole_may_exist need not be present, since
   * the vm code ensures that 0 == threads_launching and then holds
   * the lock for the duration of the VM operation.  it is safer this
   * way, in case we later introduce code that might want to
   * temporarily drop the process lock.
   */
  int                       vm_hole_may_exist;
  int                       threads_launching;
#endif

  /* Array of NaCl syscall handlers. */
  struct NaClSyscallTableEntry syscall_table[NACL_MAX_SYSCALLS];

  struct NaClResourceNaClApp        resources;

  /*
   * The ordering in this enum is important. We use the ordering
   * to check that the status of module initialization; the state
   * is really being used as a state machine. Please do not change
   * the ordering, if you need to add a new state please do so at
   * the appropriate position dependending on a module loading phase.
   */

  enum NaClModuleInitializationState {
    NACL_MODULE_UNINITIALIZED = 0,
    NACL_MODULE_LOADING,
    NACL_MODULE_LOADED,
    NACL_MODULE_STARTING,
    NACL_MODULE_STARTED,
    NACL_MODULE_ERROR
  }                                 module_initialization_state;
  NaClErrorCode                     module_load_status;

  /*
   * runtime info below, thread state, etc; initialized only when app
   * is run.  Mutex mu protects access to mem_map and other member
   * variables while the application is running and may be
   * multithreaded; thread, desc members have their own locks.  At
   * other times it is assumed that only one thread is
   * constructing/loading the NaClApp and that no mutual exclusion is
   * needed.
   */

  /*
   * memory map is in user addresses.
   */
  struct NaClVmmap          mem_map;

  struct NaClIntervalMultiset *mem_io_regions;

  /*
   * This is the effector interface object that is used to manipulate
   * NaCl apps by the objects in the NaClDesc class hierarchy.  This
   * is used by this NaClApp when making NaClDesc method calls from
   * syscall handlers.  Currently, this is when NaClDesc objects need
   * to manipulate the untrusted address space -- the mmap
   * implementation need to unmap the untrusted pages, and on Windows
   * this requires different calls depending on how the pages were
   * created.
   */
  struct NaClDescEffector   *effp;

  /*
   * may reject nexes that are incompatible w/ dynamic-text in the near future
   */
  int                       enable_dyncode_syscalls;
  int                       use_shm_for_dynamic_text;
  struct NaClDesc           *text_shm;
  struct NaClMutex          dynamic_load_mutex;
  /*
   * This records which pages in text_shm have been allocated.  When a
   * page is allocated, it is filled with halt instructions and then
   * made executable by untrusted code.
   */
  uint8_t                   *dynamic_page_bitmap;

  /*
   * The array of dynamic_regions is maintained in sorted order
   * Accesses must be protected by dynamic_load_mutex.
   */
  struct NaClDynamicRegion  *dynamic_regions;
  int                       num_dynamic_regions;
  int                       dynamic_regions_allocated;

  /*
   * These variables are used for caching mapped writable views of the
   * dynamic text segment.  See CachedMapWritableText in nacl_text.c.
   * Accesses must be protected by dynamic_load_mutex
   */
  uint32_t                  dynamic_mapcache_offset;
  uint32_t                  dynamic_mapcache_size;
  uintptr_t                 dynamic_mapcache_ret;

  /*
   * Monotonically increasing generation number used for deletion
   * Accesses must be protected by dynamic_load_mutex
   */
  int                       dynamic_delete_generation;


  int                       running;
  int                       exit_status;

  NaClCPUFeatures           *cpu_features;
  struct NaClValidationCache *validation_cache;
  int                       ignore_validator_result;
  int                       skip_validator;
  int                       validator_stub_out_mode;

  int                       enable_list_mappings;
  /* Whether or not the app is a PNaCl app.  Boolean. */
  int                       pnacl_mode;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  uint16_t                  code_seg_sel;
  uint16_t                  data_seg_sel;
#endif

  uintptr_t                 break_addr;   /* user addr */
  /* data_end <= break_addr is an invariant */

  /*
   * Thread table lock threads_mu is higher in the locking order than
   * the thread locks, i.e., threads_mu must be acqured w/o holding
   * any per-thread lock (natp->mu).
   */
  struct NaClMutex          threads_mu;
  struct DynArray           threads;   /* NaClAppThread pointers */
  int                       num_threads;  /* number actually running */

  struct NaClFastMutex      desc_mu;
  struct DynArray           desc_tbl;  /* NaClDesc pointers */

  const struct NaClDebugCallbacks *debug_stub_callbacks;

  struct NaClDesc                 *main_nexe_desc;
  struct NaClDesc                 *irt_nexe_desc;

  struct NaClMutex          exception_mu;
  uint32_t                  exception_handler;
  int                       enable_exception_handling;
#if NACL_WINDOWS
  enum NaClDebugExceptionHandlerState debug_exception_handler_state;
  NaClAttachDebugExceptionHandlerFunc attach_debug_exception_handler_func;
#endif
  /*
   * enable_faulted_thread_queue is a boolean which enables handling
   * of untrusted faults which is used by the debug stub.  When an
   * untrusted thread faults, it is blocked until
   * NaClAppThreadUnblockIfFaulted() is called on the thread.
   */
  int                       enable_faulted_thread_queue;
  /*
   * faulted_thread_count is the number of NaClAppThreads for which
   * fault_signal is non-zero.
   */
  Atomic32                  faulted_thread_count;
#if NACL_WINDOWS
  /*
   * An event that is signaled by debug exception handler process when it fills
   * fault_signal field with non-zero value for some NaClAppThread.
   */
  HANDLE                    faulted_thread_event;
#else
  /*
   * A file descriptor of a pipe which becomes available for reading in
   * the event that fault_signal for some NaClAppThread becomes non-zero.
   */
  int                       faulted_thread_fd_read;
  int                       faulted_thread_fd_write;
#endif

  /*
   * Cache of sysconf(_SC_NPROCESSORS_ONLN) (or equivalent) result.
   */
  int sc_nprocessors_onln;

  const struct NaClValidatorInterface *validator;

#if !NACL_LINUX
  /*
   * Mutex for protecting futex_wait_list_head.  Lock ordering:
   * NaClApp::mu may be claimed after futex_wait_list_mu but never
   * before it.
   */
  struct NaClMutex          futex_wait_list_mu;
  /*
   * This is the sentinel node for a doubly linked list of
   * NaClAppThreads.  This lists the threads that are waiting to be
   * woken up by futex_wake().  This list must only be accessed while
   * holding the mutex futex_wait_list_mu.
   */
  struct NaClListNode       futex_wait_list_head;
#endif
};



void  NaClAppIncrVerbosity(void);

/*
 * Standard Ctor for NaClApp objects.  Installs default syscall
 * handlers.
 *
 * If invoked after the outer sandbox is enabled, the caller is
 * responsible for initializing the sc_nprocessors_onln member to a
 * sane value.
 *
 * nap is a pointer to the NaCl object that is being filled in.
 */
int NaClAppCtor(struct NaClApp  *nap) NACL_WUR;

/*
 * This is the same as NaClAppCtor(), except that it initializes the
 * NaClApp without registering any syscall handlers.  Syscall handlers
 * can be added explicitly using NACL_REGISTER_SYSCALL.
 */
int NaClAppWithEmptySyscallTableCtor(struct NaClApp *nap) NACL_WUR;

/*
 * Loads a NaCl ELF file into memory in preparation for running it.
 *
 * nap is a pointer to the NaCl object that is being filled in.  it
 * should be properly constructed via NaClAppCtor.
 *
 * return value: one of the LOAD_* values defined in
 * nacl_error_code.h.  TODO: add some error detail string and hang
 * that off the nap object, so that more details are available w/o
 * incrementing verbosity (and polluting stdout).
 *
 * note: it may be necessary to flush the icache if the memory
 * allocated for use had already made it into the icache from another
 * NaCl application instance, and the icache does not detect
 * self-modifying code / data writes and automatically invalidate the
 * cache lines.
 */
NaClErrorCode NaClAppLoadFile(struct NaClDesc *ndp,
                              struct NaClApp *nap) NACL_WUR;

/*
 * Just like NaClAppLoadFile, but allow control over ASLR.
 */
NaClErrorCode NaClAppLoadFileAslr(struct NaClDesc *ndp,
                                  struct NaClApp *nap,
                                  enum NaClAslrMode aslr_mode) NACL_WUR;


NaClErrorCode NaClAppLoadFileDynamically(
    struct NaClApp *nap,
    struct NaClDesc *ndp,
    struct NaClValidationMetadata *metadata) NACL_WUR;

int NaClValidateCode(struct NaClApp *nap,
                     uintptr_t      guest_addr,
                     uint8_t        *data,
                     size_t         size,
                     const struct NaClValidationMetadata *metadata) NACL_WUR;

/*
 * Validates that the code found at data_old can safely be replaced with
 * the code found at data_new.
 */
int NaClValidateCodeReplacement(struct    NaClApp *nap,
                                uintptr_t guest_addr,
                                uint8_t   *data_old,
                                uint8_t   *data_new,
                                size_t    size);

/*
 * Copies code from data_new to data_old in a thread-safe way.
 */
int NaClCopyCode(struct NaClApp *nap, uintptr_t guest_addr,
                 uint8_t *data_old, uint8_t *data_new,
                 size_t size);

/*
 * Copies an instruction in a thread-safe way. Used by validators.
 */
int NaClCopyInstruction(uint8_t *dst, uint8_t *src, uint8_t sz);

NaClErrorCode NaClValidateImage(struct NaClApp  *nap) NACL_WUR;


int NaClAddrIsValidEntryPt(struct NaClApp *nap,
                           uintptr_t      addr);

int NaClAddrIsValidIrtEntryPt(struct NaClApp *nap,
                              uintptr_t      addr);

/*
 * Takes ownership of descriptor, i.e., when NaCl app closes, it's gone.
 */
void NaClAddHostDescriptor(struct NaClApp *nap,
                           int            host_os_desc,
                           int            mode,
                           int            nacl_desc);

/*
 * Takes ownership of handle.
 */
void NaClAddImcHandle(struct NaClApp  *nap,
                      NaClHandle      h,
                      int             nacl_desc);

/*
 * Report the low eight bits of |exit_status| via the reverse channel
 * in |nap|, if one exists, to whomever is interested.  This usually
 * involves an RPC.  Returns true if successfully reported.
 *
 * Also mark nap's exit_status and running member variables, announce
 * via condvar that the nexe should be considered no longer running.
 *
 * Returns true (non-zero) if exit status was reported via the reverse
 * channel, and false (0) otherwise.
 */
int NaClReportExitStatus(struct NaClApp *nap, int exit_status);

/*
 * Get the top of the initial thread's stack.  Returns a user address.
 */
uintptr_t NaClGetInitialStackTop(struct NaClApp *nap);

/*
 * Used to launch the main thread.  NB: calling thread may in the
 * future become the main NaCl app thread, and this function will
 * return only after the NaCl app main thread exits.  In such an
 * alternative design, NaClWaitForMainThreadToExit will become a
 * no-op.
 */
int NaClCreateMainThread(struct NaClApp     *nap,
                         int                argc,
                         char               **argv,
                         char const *const  *envp) NACL_WUR;

int NaClWaitForMainThreadToExit(struct NaClApp  *nap);

/*
 * Used by syscall code.
 */
int32_t NaClCreateAdditionalThread(struct NaClApp *nap,
                                   uintptr_t      prog_ctr,
                                   uintptr_t      stack_ptr,
                                   uint32_t       user_tls1,
                                   uint32_t       user_tls2) NACL_WUR;

void NaClLoadTrampoline(struct NaClApp *nap, enum NaClAslrMode aslr_mode);

void NaClLoadSpringboard(struct NaClApp  *nap);

static const uintptr_t kNaClBadAddress = (uintptr_t) -1;

#include "native_client/src/trusted/service_runtime/sel_ldr-inl.h"

/*
 * Looks up a descriptor in the open-file table.  An additional
 * reference is taken on the returned NaClDesc object (if non-NULL).
 * The caller is responsible for invoking NaClDescUnref() on it when
 * done.
 */
struct NaClDesc *NaClAppGetDesc(struct NaClApp *nap,
                                int            d);

/* NaClAppSetDesc() is defined in src/public/chrome_main.h. */

int32_t NaClAppSetDescAvail(struct NaClApp   *nap,
                            struct NaClDesc  *ndp);

/*
 * Versions that are called while already holding the desc_mu lock
 */
struct NaClDesc *NaClAppGetDescMu(struct NaClApp *nap,
                                  int            d);

void NaClAppSetDescMu(struct NaClApp   *nap,
                      int              d,
                      struct NaClDesc  *ndp);

int32_t NaClAppSetDescAvailMu(struct NaClApp   *nap,
                              struct NaClDesc  *ndp);


int NaClAddThread(struct NaClApp        *nap,
                  struct NaClAppThread  *natp);

int NaClAddThreadMu(struct NaClApp        *nap,
                    struct NaClAppThread  *natp);

void NaClRemoveThread(struct NaClApp  *nap,
                      int             thread_num);

void NaClRemoveThreadMu(struct NaClApp  *nap,
                        int             thread_num);

struct NaClAppThread *NaClGetThreadMu(struct NaClApp  *nap,
                                      int             thread_num);

void NaClAppInitialDescriptorHookup(struct NaClApp  *nap);

/*
 * Loads the |nexe| as a NaCl app module.
 */
void NaClAppLoadModule(struct NaClApp *self, struct NaClDesc *nexe);

/*
 * Starts the NaCl app.
 */
void NaClAppStartModule(struct NaClApp *self);

NaClErrorCode NaClGetLoadStatus(struct NaClApp *nap) NACL_WUR;

void NaClFillMemoryRegionWithHalt(void *start, size_t size);

void NaClFillTrampolineRegion(struct NaClApp *nap);

void NaClFillEndOfTextRegion(struct NaClApp *nap);

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32

int NaClMakePcrelThunk(struct NaClApp *nap, enum NaClAslrMode aslr_mode);

#endif

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64

int NaClMakeDispatchAddrs(struct NaClApp *nap);

void NaClPatchOneTrampolineCall(uintptr_t call_target_addr,
                                uintptr_t target_addr);

#endif

void NaClPatchOneTrampoline(struct NaClApp *nap,
                            uintptr_t target_addr);
/*
 * target is an absolute address in the source region.  the patch code
 * will figure out the corresponding address in the destination region
 * and modify as appropriate.  this makes it easier to specify, since
 * the target is typically the address of some symbol from the source
 * template.
 */
struct NaClPatch {
  uintptr_t           target;
  uint64_t            value;
};

struct NaClPatchInfo {
  uintptr_t           dst;
  uintptr_t           src;
  size_t              nbytes;

  struct NaClPatch    *abs16;
  size_t              num_abs16;

  struct NaClPatch    *abs32;
  size_t              num_abs32;

  struct NaClPatch    *abs64;
  size_t              num_abs64;

#if NACL_BUILD_SUBARCH == 32
  uintptr_t           *rel32;
  size_t              num_rel32;
#endif

  uintptr_t           *rel64;
  size_t              num_rel64;
};

struct NaClPatchInfo *NaClPatchInfoCtor(struct NaClPatchInfo *self);

void NaClApplyPatchToMemory(struct NaClPatchInfo *patch);

int NaClAppThreadInitArchSpecific(struct NaClAppThread *natp,
                                  nacl_reg_t           prog_ctr,
                                  nacl_reg_t           stack_ptr);

void NaClVmHoleWaitToStartThread(struct NaClApp *nap);

void NaClVmHoleThreadStackIsSafe(struct NaClApp *nap);

void NaClVmHoleOpeningMu(struct NaClApp *nap);

void NaClVmHoleClosingMu(struct NaClApp *nap);

/*
 * More VM race detection.  In Windows, when we unmap or mmap over
 * existing memory, we cannot maintain the address space reservation
 * -- we have to unmap the original mapping with the appropriate
 * unmapping function (VirtualFree or UnmapViewOfFile) before we can
 * VirtualAlloc to reserve the addresss pace, leaving a timing window
 * where another thread (possibly injected into the binary, e.g.,
 * antivirus code) might map something else into.  We stop user-space
 * threads when mmap/munmap occurs and detect if the temporary VM hole
 * was filled by some other thread (and abort the process in that
 * case), so that untrusted code cannot observe nor modify the memory
 * that lands in the hole.  However, we still have the case where the
 * untrusted code's threads aren't running user-space code -- a thread
 * may have entered into a syscall handler.  We don't stop such
 * threads, because they might be holding locks that the memory
 * mapping functions need.  This means that, for example, a malicious
 * application could -- assuming it could create the innocent thread
 * race condition -- have one thread invoke the "write" NaCl syscall,
 * enter into the host OS "write" syscall code in the NaCl syscall
 * handler, then have another thread mmap (or munmap) the memory where
 * the source memory region from which the content to be written
 * resides, and as a side effect of this race, exfiltrate memory
 * contents that NaCl modules aren't supposed to be able to access.
 *
 * NB: we do not try to prevent data races such as two "read" syscalls
 * simultaneously trying to write the same memory region, or
 * concurrent "read" and "write" syscalls racing on the same memory
 * region..
 */

/*
 * Some potentially blocking I/O operation is about to start.  Syscall
 * handlers implement DMA-style access where the host-OS syscalls
 * directly read/write untrusted memory, so we must record the
 * affected memory ranges as "in use" by I/O operations.
 */
void NaClVmIoWillStart(struct NaClApp *nap,
                       uint32_t addr_first_usr,
                       uint32_t addr_last_usr);


/*
 * It is a fatal error to have an invocation of NaClVmIoHasEnded whose
 * arguments do not match those of an earlier, unmatched invocation of
 * NaClVmIoWillStart.
 */
void NaClVmIoHasEnded(struct NaClApp *nap,
                      uint32_t addr_first_usr,
                      uint32_t addr_last_usr);

/*
 * Used by operations (mmap, munmap) that will open a VM hole.
 * Invoked while holding the VM lock.  Check that no I/O is pending;
 * abort the app if the app is racing I/O operations against VM
 * operations.
 */
void NaClVmIoPendingCheck_mu(struct NaClApp *nap,
                             uint32_t addr_first_usr,
                             uint32_t addr_last_usr);

void NaClGdbHook(struct NaClApp const *nap);

#if NACL_LINUX
void NaClHandleBootstrapArgs(int *argc_p, char ***argv_p);
void NaClHandleRDebug(const char *switch_value, char *argv0);
void NaClHandleReservedAtZero(const char *switch_value);
#else
static INLINE void NaClHandleBootstrapArgs(int *argc_p, char ***argv_p) {
  UNREFERENCED_PARAMETER(argc_p);
  UNREFERENCED_PARAMETER(argv_p);
}
#endif

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_H_ */
