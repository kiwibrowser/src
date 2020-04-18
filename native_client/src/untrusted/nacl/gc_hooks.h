/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_GC_HOOKS_H
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_GC_HOOKS_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*TYPE_nacl_gc_hook)(void);

extern void nacl_register_gc_hooks(TYPE_nacl_gc_hook prehook,
                                   TYPE_nacl_gc_hook posthook);


#if defined(__cplusplus)
}
#endif

#endif  /* NATIVE_CLIENT_SRC_UNTRUSTED_NACL_GC_HOOKS_H */
