/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/osx/mach_exception_handler.h"

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_status.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_exc.h"
#include "native_client/src/trusted/service_runtime/nacl_exception.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/osx/crash_filter.h"
#include "native_client/src/trusted/service_runtime/osx/mach_thread_map.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86


/*
 * MIG generated message pump from /usr/include/mach/exc.defs
 * Tweaked to place in an isolated namespace.
 */
boolean_t nacl_exc_server(
    mach_msg_header_t *InHeadP,
    mach_msg_header_t *OutHeadP);


/* Exception types to intercept. */
#define NACL_MACH_EXCEPTION_MASK \
    (EXC_MASK_BAD_ACCESS | \
     EXC_MASK_BAD_INSTRUCTION | \
     EXC_MASK_ARITHMETIC | \
     EXC_MASK_BREAKPOINT)

/*
 * <mach/mach_types.defs> says that the arrays in MachExceptionParameters have
 * room for 32 elements.  EXC_TYPES_COUNT, a smaller value, has grown over
 * time in major SDK releases.  task_get_exception_ports expects there to be
 * room for all 32 and is not bounded by EXC_TYPES_COUNT or by the initial
 * value of its handler_count argument, contrary to its documentation.
 * Provide a constant matching its limit to avoid potential buffer overflow.
 */
#define NACL_MAX_EXCEPTION_PORTS 32

struct MachExceptionParameters {
  mach_msg_type_number_t count;
  exception_mask_t masks[NACL_MAX_EXCEPTION_PORTS];
  mach_port_t ports[NACL_MAX_EXCEPTION_PORTS];
  exception_behavior_t behaviors[NACL_MAX_EXCEPTION_PORTS];
  thread_state_flavor_t flavors[NACL_MAX_EXCEPTION_PORTS];
};

struct MachExceptionHandlerData {
  struct MachExceptionParameters old_ports;
  mach_port_t exception_port;
};

/*
 * This is global because MIG does not provide a mechanism to associate
 * user data with a handler. We could 'sed' the output, but that might be
 * brittle. This is moot as the handler is task wide. Revisit if we switch to
 * a per-thread handler.
 */
static struct MachExceptionHandlerData *g_MachExceptionHandlerData = 0;


static int ExceptionCodeToNaClSignalNumber(exception_type_t exception) {
  switch (exception) {
    case EXC_BREAKPOINT:
      return NACL_ABI_SIGTRAP;
    default:
      return NACL_ABI_SIGSEGV;
  }
}

static void FireDebugStubEvent(int pipe_fd) {
  char buf = 0;
  if (write(pipe_fd, &buf, sizeof(buf)) != sizeof(buf)) {
    NaClLog(LOG_FATAL, "FireDebugStubEvent: Can't send debug stub event\n");
  }
}

#if NACL_BUILD_SUBARCH == 32

#define NATIVE_x86_THREAD_STATE x86_THREAD_STATE32
#define X86_REG_SP(regs)    ((regs)->uts.ts32.__esp)
#define X86_REG_IP(regs)    ((regs)->uts.ts32.__eip)
#define X86_REG_FLAGS(regs) ((regs)->uts.ts32.__eflags)

#elif NACL_BUILD_SUBARCH == 64

#define NATIVE_x86_THREAD_STATE x86_THREAD_STATE64
#define X86_REG_SP(regs)    ((regs)->uts.ts64.__rsp)
#define X86_REG_IP(regs)    ((regs)->uts.ts64.__rip)
#define X86_REG_FLAGS(regs) ((regs)->uts.ts64.__rflags)

#endif  /* NACL_BUILD_SUBARCH */

enum HandleExceptionResult {
  kHandleExceptionUnhandled = 0,
  kHandleExceptionHandled_SetState,
  kHandleExceptionHandled_DontSetState
};

static enum HandleExceptionResult HandleException(mach_port_t thread_port,
                                                  exception_type_t exception,
                                                  int *is_untrusted,
                                                  x86_thread_state_t *regs) {
  kern_return_t result;
  uint32_t nacl_thread_index;
  struct NaClApp *nap;
  struct NaClAppThread *natp;
  struct NaClExceptionFrame frame;
  uint32_t frame_addr_user;
  uintptr_t frame_addr_sys;
  struct NaClSignalContext sig_context;
#if NACL_BUILD_SUBARCH == 32
  uint16_t trusted_cs = NaClGetGlobalCs();
  uint16_t trusted_ds = NaClGetGlobalDs();
#endif

  CHECK(regs->tsh.flavor == NATIVE_x86_THREAD_STATE);

  /* Assume untrusted crash until we know otherwise. */
  *is_untrusted = TRUE;

#if NACL_BUILD_SUBARCH == 32
  /*
   * We can get the thread index from the segment selector used for TLS
   * from %gs >> 3.
   * TODO(bradnelson): Migrate that knowledge to a single shared location.
   */
  nacl_thread_index = regs->uts.ts32.__gs >> 3;
#elif NACL_BUILD_SUBARCH == 64
  nacl_thread_index = NaClGetThreadIndexForMachThread(thread_port);

  if (nacl_thread_index == NACL_TLS_INDEX_INVALID) {
    *is_untrusted = FALSE;
    return kHandleExceptionUnhandled;
  }
#endif

  natp = NaClAppThreadGetFromIndex(nacl_thread_index);
  if (natp == NULL) {
    *is_untrusted = FALSE;
    return kHandleExceptionUnhandled;
  }
  nap = natp->nap;

  /*
   * TODO(bradnelson): For x86_32, this makes the potentially false assumption
   *     that cs is the last thing to change when switching into untrusted
   *     code. We need tests to vet this.
   */
  *is_untrusted = NaClMachThreadStateIsInUntrusted(regs, nacl_thread_index);

  /*
   * If trusted code accidentally jumped to untrusted code, don't let the
   * untrusted exception handler take over.
   */
  if (*is_untrusted &&
      (natp->suspend_state & NACL_APP_THREAD_UNTRUSTED) == 0) {
    *is_untrusted = 0;
    return kHandleExceptionUnhandled;
  }

  if (!*is_untrusted) {
#if NACL_BUILD_SUBARCH == 32
    /*
     * If we are single-stepping, allow NaClSwitchRemainingRegsViaECX()
     * to continue in order to restore control to untrusted code.
     */
    if (exception == EXC_BREAKPOINT &&
        (X86_REG_FLAGS(regs) & NACL_X86_TRAP_FLAG) != 0 &&
        X86_REG_IP(regs) >= (uintptr_t) NaClSwitchRemainingRegsViaECX &&
        X86_REG_IP(regs) < (uintptr_t) NaClSwitchRemainingRegsAsmEnd) {
      return kHandleExceptionHandled_DontSetState;
    }
#endif
    return kHandleExceptionUnhandled;
  }

  if (nap->enable_faulted_thread_queue) {
#if NACL_BUILD_SUBARCH == 32
    /*
     * If we are single-stepping, step through until we reach untrusted code.
     */
    if (exception == EXC_BREAKPOINT &&
        (X86_REG_FLAGS(regs) & NACL_X86_TRAP_FLAG) != 0) {
      if (X86_REG_IP(regs) >= nap->all_regs_springboard.start_addr &&
          X86_REG_IP(regs) < nap->all_regs_springboard.end_addr) {
        return kHandleExceptionHandled_DontSetState;
      }
      /*
       * Step through the instruction we have been asked to restore
       * control to.
       */
      if (X86_REG_IP(regs) == natp->user.gs_segment.new_prog_ctr) {
        return kHandleExceptionHandled_DontSetState;
      }
    }
#endif

    /*
     * Increment the kernel's thread suspension count so that the
     * thread remains suspended after we return.
     */
    result = thread_suspend(thread_port);
    if (result != KERN_SUCCESS) {
      NaClLog(LOG_FATAL, "HandleException: thread_suspend() call failed\n");
    }
    /*
     * Notify the handler running on another thread.  This must happen
     * after the thread_suspend() call, otherwise the handler might
     * receive the notification and attempt to decrement the thread's
     * suspension count before we have incremented it.
     */
    natp->fault_signal = ExceptionCodeToNaClSignalNumber(exception);
    AtomicIncrement(&nap->faulted_thread_count, 1);
    FireDebugStubEvent(nap->faulted_thread_fd_write);
    return kHandleExceptionHandled_DontSetState;
  }

  if (exception != EXC_BAD_ACCESS &&
      exception != EXC_BAD_INSTRUCTION &&
      exception != EXC_ARITHMETIC) {
    return kHandleExceptionUnhandled;
  }

  NaClSignalContextFromMacThreadState(&sig_context, regs);
  if (!NaClSignalCheckSandboxInvariants(&sig_context, natp)) {
    return kHandleExceptionUnhandled;
  }

  /* Don't handle if no exception handler is set. */
  if (nap->exception_handler == 0) {
    return kHandleExceptionUnhandled;
  }

  /* Don't handle it if the exception flag is set. */
  if (natp->exception_flag) {
    return kHandleExceptionUnhandled;
  }
  /* Set the flag. */
  natp->exception_flag = 1;

  /* Get location of exception stack frame. */
  if (natp->exception_stack) {
    frame_addr_user = natp->exception_stack;
  } else {
    /* If not set default to user stack. */
    frame_addr_user = X86_REG_SP(regs) - NACL_STACK_RED_ZONE;
  }

  /* Align stack frame properly. */
  frame_addr_user -=
      sizeof(struct NaClExceptionFrame) - NACL_STACK_PAD_BELOW_ALIGN;
  frame_addr_user &= ~NACL_STACK_ALIGN_MASK;
  frame_addr_user -= NACL_STACK_PAD_BELOW_ALIGN;

  /* Convert from user to system space. */
  frame_addr_sys = NaClUserToSysAddrRange(
      nap, frame_addr_user, sizeof(struct NaClExceptionFrame));
  if (frame_addr_sys == kNaClBadAddress) {
    return kHandleExceptionUnhandled;
  }

  /* Set up the stack frame for the handler invocation. */
  NaClSignalSetUpExceptionFrame(
      &frame, &sig_context,
      frame_addr_user + offsetof(struct NaClExceptionFrame, context));

  /*
   * Write the stack frame into untrusted address space.  We do not
   * write to the memory directly because that will fault if the
   * destination location is not writable.  Faulting is OK for NaCl
   * syscalls, but here we do not want to trigger an exception while
   * in the exception handler.  The overhead of using a Mach system
   * call to write to memory, 2-3 microseconds on a 2.3GHz 4-core i7,
   * is acceptable here.
   */
  result = mach_vm_write(mach_task_self(), frame_addr_sys,
                         (uintptr_t) &frame, sizeof(frame));
  if (result != KERN_SUCCESS) {
    return kHandleExceptionUnhandled;
  }

  /* Set up thread context to resume at handler. */
  /* TODO(bradnelson): put all registers in some default state. */
#if NACL_BUILD_SUBARCH == 32
  /*
   * Put registers in right place to land at NaClSwitchNoSSEViaECX
   * This is required because:
   *   - For an unknown reason thread_set_state resets %cs to the default
   *     value, even when set to something else, in current XNU versions.
   *   - An examination of the XNU sources indicates
   *     that setting the code which state the thread state resets
   *     %cs, %ds, %es, %ss to their default values in some early versions.
   *     (For instance: xnu-792.6.22/osfmk/i386/pcb.c:616)
   * This precludes going directly to the untrusted handler.
   * Instead we call a variant of NaClSwitchNoSSE which takes a pointer
   * to the thread user context in %ecx.
   */
  natp->user.new_prog_ctr = nap->exception_handler;
  natp->user.stack_ptr = frame_addr_user;
  X86_REG_IP(regs) = (uint32_t) &NaClSwitchNoSSEViaECX;
  regs->uts.ts32.__cs = trusted_cs;
  regs->uts.ts32.__ecx = (uint32_t) &natp->user;
  regs->uts.ts32.__ds = trusted_ds;
  regs->uts.ts32.__es = trusted_ds;  /* just for good measure */
  regs->uts.ts32.__ss = trusted_ds;  /* just for good measure */
#elif NACL_BUILD_SUBARCH == 64
  X86_REG_IP(regs) = NaClUserToSys(nap, nap->exception_handler);
  X86_REG_SP(regs) = frame_addr_sys;

  /* Argument 1 */
  regs->uts.ts64.__rdi = frame_addr_user +
                         offsetof(struct NaClExceptionFrame, context);
#endif
  X86_REG_FLAGS(regs) &= ~NACL_X86_DIRECTION_FLAG;

  /* Return success, and resume the thread. */
  return kHandleExceptionHandled_SetState;
}


static kern_return_t ForwardException(
    struct MachExceptionHandlerData *data,
    mach_port_t thread,
    mach_port_t task,
    exception_type_t exception,
    exception_data_t code,
    mach_msg_type_number_t code_count) {
  unsigned int i;
  mach_port_t target_port;
  exception_behavior_t target_behavior;
  kern_return_t kr;

  /* Find a port with a mask matching this exception to pass it on to. */
  for (i = 0; i < data->old_ports.count; ++i) {
    if (data->old_ports.masks[i] & (1 << exception)) {
      break;
    }
  }
  if (i == data->old_ports.count) {
    return KERN_FAILURE;
  }
  target_port = data->old_ports.ports[i];
  target_behavior = data->old_ports.behaviors[i];

  /*
   * By default a null exception port is registered.
   * As it is unclear how to forward to this, we should just fail.
   */
  if (target_port == MACH_PORT_NULL) {
    return KERN_FAILURE;
  }

  /*
   * Only support EXCEPTION_DEFAULT, as we only plan to inter-operate with
   * Breakpad for now.
   */
  CHECK(target_behavior == EXCEPTION_DEFAULT);

  /* Forward the exception. */
  kr = exception_raise(target_port, thread, task, exception, code, code_count);

  /*
   * Don't set the thread state. See the comment in
   * nacl_catch_exception_raise_state_identity. Transforming KERN_SUCCESS to
   * MACH_RCV_PORT_DIED is necessary because the exception was delivered with
   * behavior EXCEPTION_STATE_IDENTITY to
   * nacl_catch_exception_raise_state_identity, but it was forwarded to a
   * handler with behavior EXCEPTION_DEFAULT via exception_raise, and such
   * handlers don't provide a new thread state. They can set a new thread
   * state on their own by calling thread_set_state. Returning KERN_SUCCESS
   * would allow the old state passed to
   * nacl_catch_exception_raise_state_identity to overwrite any new state set
   * by the handler that the exception was forwarded to via exception_raise.
   */
  return kr == KERN_SUCCESS ? MACH_RCV_PORT_DIED : kr;
}


kern_return_t nacl_catch_exception_raise(
    mach_port_t exception_port,
    mach_port_t thread,
    mach_port_t task,
    exception_type_t exception,
    exception_data_t code,
    mach_msg_type_number_t code_count) {
  /* MIG generated code expects this, but should never be called. */
  UNREFERENCED_PARAMETER(exception_port);
  UNREFERENCED_PARAMETER(thread);
  UNREFERENCED_PARAMETER(task);
  UNREFERENCED_PARAMETER(exception);
  UNREFERENCED_PARAMETER(code);
  UNREFERENCED_PARAMETER(code_count);
  NaClLog(LOG_FATAL, "nacl_catch_exception_raise: "
                     "Unrequested message received.\n");
  return KERN_FAILURE;
}

kern_return_t nacl_catch_exception_raise_state(
    mach_port_t exception_port,
    exception_type_t exception,
    const exception_data_t code,
    mach_msg_type_number_t code_count,
    int *flavor,
    const thread_state_t old_state,
    mach_msg_type_number_t old_state_count,
    thread_state_t new_state,
    mach_msg_type_number_t *new_state_count) {
  /* MIG generated code expects this, but should never be called. */
  UNREFERENCED_PARAMETER(exception_port);
  UNREFERENCED_PARAMETER(exception);
  UNREFERENCED_PARAMETER(code);
  UNREFERENCED_PARAMETER(code_count);
  UNREFERENCED_PARAMETER(flavor);
  UNREFERENCED_PARAMETER(old_state);
  UNREFERENCED_PARAMETER(old_state_count);
  UNREFERENCED_PARAMETER(new_state);
  UNREFERENCED_PARAMETER(new_state_count);
  NaClLog(LOG_FATAL, "nacl_catch_exception_raise_state: "
                     "Unrequested message received.\n");
  return KERN_FAILURE;
}

static void PrintCrashMessage(int exception_type, int is_untrusted,
                              x86_thread_state_t *regs) {
  char buf[128];
  int len = snprintf(
      buf, sizeof(buf),
      "\n** Mach exception %d from %s code: pc=%" NACL_PRIx64 "\n",
      exception_type,
      is_untrusted ? "untrusted" : "trusted",
      (uint64_t) X86_REG_IP(regs));
  write(2, buf, len);
}

kern_return_t nacl_catch_exception_raise_state_identity (
        mach_port_t exception_port,
        mach_port_t thread,
        mach_port_t task,
        exception_type_t exception,
        exception_data_t code,
        mach_msg_type_number_t code_count,
        int *flavor,
        thread_state_t old_state,
        mach_msg_type_number_t old_state_count,
        thread_state_t new_state,
        mach_msg_type_number_t *new_state_count) {
  int rv;
  int is_untrusted;

  DCHECK(exception_port == g_MachExceptionHandlerData->exception_port);

  CHECK(*flavor == x86_THREAD_STATE);
  CHECK(old_state_count == x86_THREAD_STATE_COUNT);
  CHECK(*new_state_count >= x86_THREAD_STATE_COUNT);
  *new_state_count = x86_THREAD_STATE_COUNT;
  memcpy(new_state, old_state, x86_THREAD_STATE_COUNT * sizeof(natural_t));

  /* Check if we want to handle this exception. */
  rv = HandleException(thread,
                       exception,
                       &is_untrusted,
                       (x86_thread_state_t *) new_state);
  if (rv == kHandleExceptionHandled_SetState) {
    return KERN_SUCCESS;
  } else if (rv == kHandleExceptionHandled_DontSetState) {
    /*
     * To avoid setting the thread state, return MACH_RCV_PORT_DIED. In
     * exception_deliver, the kernel will only set the thread state if
     * exception_raise_state returns MACH_MSG_SUCCESS (== KERN_SUCCESS), so
     * another value is needed to avoid having it set the state. KERN_SUCCESS
     * and MACH_RCV_PORT_DIED are the only two values that don't result in task
     * termination in exception_triage. See 10.8.2
     * xnu-2050.18.24/osfmk/kern/exception.c.
     *
     * This is done instead of letting the kernel set the new thread state to
     * be the same as the old state because the kernel resets %cs to the
     * default value when setting a thread's state. This behavior is explained
     * in more detail in HandleException.
     */
    return MACH_RCV_PORT_DIED;
  }

  PrintCrashMessage(exception, is_untrusted, (x86_thread_state_t *) old_state);

  /*
   * Don't forward if the crash is untrusted, but unhandled.
   * (As we don't want things like Breakpad handling the crash.)
   */
  if (is_untrusted) {
    return KERN_FAILURE;
  }

  /* Forward on the exception to the old set of ports. */
  return ForwardException(
      g_MachExceptionHandlerData, thread, task, exception, code, code_count);
}

static void *MachExceptionHandlerThread(void *arg) {
  struct MachExceptionHandlerData *data =
      (struct MachExceptionHandlerData *) arg;
  kern_return_t result;
  union {
    mach_msg_header_t header;
    union __RequestUnion__nacl_exc_subsystem nacl_exc_subsystem_request;
  } request;
  union {
    mach_msg_header_t header;
    union __ReplyUnion__nacl_exc_subsystem nacl_exc_subsystem_reply;
  } reply;

  for (;;) {
    result = mach_msg(&request.header, MACH_RCV_MSG | MACH_RCV_LARGE, 0,
                      sizeof(request), data->exception_port,
                      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (result != MACH_MSG_SUCCESS) {
      goto failure;
    }
    if (!nacl_exc_server(&request.header, &reply.header)) {
      goto failure;
    }
    result = mach_msg(&reply.header, MACH_SEND_MSG,
                      reply.header.msgh_size, 0,
                      MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (result != MACH_MSG_SUCCESS) {
      goto failure;
    }
  }

failure:
  free(data);

  return 0;
}

static int InstallHandler(struct MachExceptionHandlerData *data) {
  kern_return_t result;
  mach_port_t current_task = mach_task_self();
  unsigned int i;

  /* Capture old handler info. */
  data->old_ports.count = NACL_MAX_EXCEPTION_PORTS;
  result = task_get_exception_ports(current_task, NACL_MACH_EXCEPTION_MASK,
                                    data->old_ports.masks,
                                    &data->old_ports.count,
                                    data->old_ports.ports,
                                    data->old_ports.behaviors,
                                    data->old_ports.flavors);
  if (result != KERN_SUCCESS) {
    return result;
  }

  /*
   * We only handle forwarding of the EXCEPTION_DEFAULT behavior (all that
   * Breakpad needs). Check that all old handlers are either of this behavior
   * type or null.
   *
   * NOTE: Ideally we might also require a particular behavior for null
   * exception ports. Unfortunately, testing indicates that while on
   * OSX 10.6 / 10.7 the behavior for such a null port is set to 0,
   * on OSX 10.5 it is set to 0x803fe956 (on a given run).
   * As tasks inherit exception ports from their parents, this may be
   * an uninitialized value carried along from a parent.
   * http://opensource.apple.com/source/xnu/xnu-1228.0.2/osfmk/kern/ipc_tt.c
   * For now, we will ignore the behavior when the port is null.
   */
  for (i = 0; i < data->old_ports.count; ++i) {
    CHECK(data->old_ports.behaviors[i] == EXCEPTION_DEFAULT ||
          data->old_ports.ports[i] == MACH_PORT_NULL);
  }

  /* TODO(bradnelson): decide if we should set the exception port per thread. */
  /* Direct all task exceptions to new exception port. */
  result = task_set_exception_ports(current_task, NACL_MACH_EXCEPTION_MASK,
                                    data->exception_port,
                                    EXCEPTION_STATE_IDENTITY,
                                    x86_THREAD_STATE);
  if (result != KERN_SUCCESS) {
    return result;
  }

  return KERN_SUCCESS;
}

int NaClInterceptMachExceptions(void) {
  struct MachExceptionHandlerData *data;
  kern_return_t result;
  mach_port_t current_task;
  pthread_attr_t attr;
  int thread_result;
  pthread_t exception_handler_thread;

  current_task = mach_task_self();

  /* Allocate structure to share with exception handler thread. */
  data = (struct MachExceptionHandlerData *) calloc(1, sizeof(*data));
  if (data == NULL) {
    goto failure;
  }
  g_MachExceptionHandlerData = data;
  data->exception_port = MACH_PORT_NULL;

  /* Allocate port to receive exceptions. */
  result = mach_port_allocate(current_task, MACH_PORT_RIGHT_RECEIVE,
                              &data->exception_port);
  if (result != KERN_SUCCESS) {
    goto failure;
  }

  /* Add the right to send. */
  result = mach_port_insert_right(current_task,
                                  data->exception_port, data->exception_port,
                                  MACH_MSG_TYPE_MAKE_SEND);
  if (result != KERN_SUCCESS) {
    goto failure;
  }

  /* Install handler. */
  result = InstallHandler(data);
  if (result != KERN_SUCCESS) {
    goto failure;
  }

  /* Create handler thread. */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  thread_result = pthread_create(&exception_handler_thread, &attr,
                                 &MachExceptionHandlerThread, data);
  pthread_attr_destroy(&attr);
  if (thread_result) {
    goto failure;
  }

  return TRUE;

failure:
  if (data) {
    if (MACH_PORT_NULL != data->exception_port) {
      mach_port_deallocate(current_task, data->exception_port);
    }
    free(data);
  }
  return FALSE;
}

#endif  /* NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 */
