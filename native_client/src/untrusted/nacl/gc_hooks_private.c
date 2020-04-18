/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/gc_hooks.h"

void nacl_register_gc_hooks(TYPE_nacl_gc_hook prehook,
                            TYPE_nacl_gc_hook posthook) {
  nacl_irt_blockhook.register_block_hooks(prehook, posthook);
}
