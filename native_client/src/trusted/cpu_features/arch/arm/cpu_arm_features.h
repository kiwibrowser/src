/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file can be included many times to list all the ARM CPU features.
 * Simply define the NACL_ARM_CPU_FEATURE macro to change the behavior.
 *
 * TODO(jfb) Add other ARM CPU features.
 */

/* Some ARM CPUs leak memory when TST+LDR/TST+STR is used for sandboxing. */
NACL_ARM_CPU_FEATURE(CanUseTstMem)
