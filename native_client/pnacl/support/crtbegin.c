/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This is the native crtbegin (crtbegin.o).
 *
 * It contains the constructor/destructor for exception handling,
 * and the symbol for __EH_FRAME_BEGIN__. This is native because
 * exception handling is also native (externally provided).
 *
 * We also need to sneak in the native PPAPI shim library between
 * these bookends because otherwise the unwind tables will be messed
 * up, c.f.
 * https://chromiumcodereview.appspot.com/9909016/
 * TODO(robertm): see whether this problem goes away once we switch
 * eh_frame_headers
 */

/* This goes first to ensure that tls_params.h hasn't been #included before. */
#define NACL_DEFINE_EXTERNAL_NATIVE_SUPPORT_FUNCS
#include "native_client/src/untrusted/nacl/pnacl.h"
#include "native_client/src/untrusted/nacl/tls_params.h"

#include <stdint.h>

#include "native_client/pnacl/support/pnacl_irt.h"
#include "native_client/pnacl/support/relocate.h"


void _pnacl_wrapper_start(uint32_t *info);

/*
 * HACK:
 * The real structure is defined in unwind-dw2-fde.h
 * this is something that is at least twice as big.
 */
struct object {
  void *p[16] __attribute__((aligned(8)));
};

/*
 * __[de]register_frame_info() are provided by libgcc_eh. When not linking
 * with libgcc_eh, dummy implementations are provided.
 * See: gcc/unwind-dw2-fde.c
 */

#ifdef LINKING_WITH_LIBGCC_EH
extern void __register_frame_info(void *begin, struct object *ob);
extern void __deregister_frame_info(const void *begin);
#else /* not LINKING_WITH_LIBGCC_EH */
void __register_frame_info(void *begin, struct object *ob) {}
void __deregister_frame_info(const void *begin) {}
#endif /* LINKING_WITH_LIBGCC_EH */


/*
 * Exception handling frames are aggregated into a single section called
 * .eh_frame.  The runtime system needs to (1) have a symbol for the beginning
 * of this section, and needs to (2) mark the end of the section by a NULL.
 */

static char __EH_FRAME_BEGIN__[]
    __attribute__((section(".eh_frame"), aligned(4)))
    = { };

static void __do_eh_ctor(void) {
  static struct object object;
  __register_frame_info (__EH_FRAME_BEGIN__, &object);
}

/* This defines the entry point for a nexe produced by the PNaCl translator. */
void __pnacl_start(uint32_t *info) {
#if defined(__native_client_nonsfi__)
  apply_relocations();
#endif

  __pnacl_init_irt(info);

  /*
   * We must register exception handling unwind info before calling
   * any user code.  Note that we do not attempt to deregister the
   * unwind info at exit, but there is no particular need to do so.
   */
  __do_eh_ctor();

  _pnacl_wrapper_start(info);
}
