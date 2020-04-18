/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file can be included many times to list all the MIPS CPU features.
 * Simply define the NACL_MIPS_CPU_FEATURE macro to change the behavior.
 *
 * TODO(jfb) Add other MIPS CPU features.
 */

/* Bogus feature: needed so there's at least one.
 * Remove when others are added.
 *
 * Bogus2 feature: needed for validation_cache_test. Any data shorter than 2 is
 * considered suspicious.
 */
NACL_MIPS_CPU_FEATURE(BOGUS)
NACL_MIPS_CPU_FEATURE(BOGUS2)
