/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/gc_hooks.h"

void nacl_register_gc_hooks(TYPE_nacl_gc_hook prehook,
                            TYPE_nacl_gc_hook posthook) {
  /*
   * There is no sense in caching the result of this IRT interface
   * query because we only use the result once.
   */
  struct nacl_irt_blockhook irt_blockhook;
  __libnacl_mandatory_irt_query(NACL_IRT_BLOCKHOOK_v0_1,
                                &irt_blockhook, sizeof(irt_blockhook));
  irt_blockhook.register_block_hooks(prehook, posthook);
}
