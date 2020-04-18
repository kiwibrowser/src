/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/nacl_startup.h"


/*
 * __nacl_read_tp() is only needed on targets that don't have a
 * fast-path instruction sequence for reading the thread pointer (such
 * as %gs:0 on x86-32).  Don't define it if we don't need it, to help
 * ensure that the fast path gets used.
 */
#if defined(__x86_64__) || defined(__native_client_nonsfi__)
# define NEED_NACL_READ_TP
#endif

#if defined(NEED_NACL_READ_TP)

/*
 * TODO(mseaborn): Use the definition in nacl_config.h instead.
 * nacl_config.h is not #includable here because NACL_BUILD_ARCH
 * etc. are not defined at this point in the PNaCl toolchain build.
 */
# define NACL_SYSCALL_ADDR(syscall_number) (0x10000 + (syscall_number) * 32)

/*
 * If we are not running under the IRT, we fall back to using a
 * non-ABI-stable interface for reading the thread pointer -- either the
 * NaCl syscall (for SFI mode) or an instruction sequence (for Non-SFI
 * mode).  This is to support non-IRT-based tests and the non-IRT-using
 * sandboxed PNaCl translator.
 */
# if defined(__native_client_nonsfi__) && defined(__i386__)
static void *internal_read_tp(void) {
  void *result;
  __asm__("mov %%gs:0, %0" : "=r"(result));
  return result;
}
# elif defined(__native_client_nonsfi__) && defined(__arm__)
static void *internal_read_tp(void) {
  void *result;
  __asm__("mrc p15, 0, %0, c13, c0, 3" : "=r"(result));
  return result;
}
# else
#  define internal_read_tp (void *(*)(void)) NACL_SYSCALL_ADDR(NACL_sys_tls_get)
# endif

static void *(*g_nacl_read_tp_func)(void) = internal_read_tp;

# if defined(__arm__) && defined(__native_client_nonsfi__)
/*
 * The ARM ABI's __aeabi_read_tp() function must preserve all registers except
 * r0, but the IRT's tls_get() is just a normal function that is not
 * guaranteed to do this.  This means we need an assembly wrapper to save and
 * restore non-callee-saved registers.
 */
__asm__(".pushsection .text, \"ax\", %progbits\n"
        ".global __aeabi_read_tp\n"
        ".type __aeabi_read_tp, %function\n"
        ".arm\n"
        "__aeabi_read_tp:\n"
        "push {r1-r3, lr}\n"
        "vpush {d0-d7}\n"
        "movw r1, :lower16:(g_nacl_read_tp_func - (1f + 8))\n"
        "movt r1, :upper16:(g_nacl_read_tp_func - (1f + 8))\n"
        "1:\n"
        "ldr r0, [pc, r1]\n"
        "blx r0\n"
        "vpop {d0-d7}\n"
        "pop {r1-r3, pc}\n"
        ".popsection\n");
# else
/*
 * __nacl_read_tp is defined as a weak symbol because if a pre-translated
 * object file (which may contain calls to __nacl_read_tp) is linked with
 * the bitcode libnacl in nonpexe_tests, it will pull in libnacl's definition,
 * which would then override this one at native link time rather than causing
 * link failure.
 */
__attribute__((weak))
void *__nacl_read_tp(void) {
  return g_nacl_read_tp_func();
}
# endif

#endif

void __pnacl_init_irt(uint32_t *startup_info) {
#if defined(NEED_NACL_READ_TP)
  Elf32_auxv_t *av = nacl_startup_auxv(startup_info);

  for (; av->a_type != AT_NULL; ++av) {
    if (av->a_type == AT_SYSINFO) {
      TYPE_nacl_irt_query irt_query = (TYPE_nacl_irt_query) av->a_un.a_val;
      struct nacl_irt_tls irt_tls;
      if (irt_query(NACL_IRT_TLS_v0_1, &irt_tls, sizeof(irt_tls))
          == sizeof(irt_tls)) {
        g_nacl_read_tp_func = irt_tls.tls_get;
      }
      return;
    }
  }
#endif
}
