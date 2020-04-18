/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"


/*
 * Needed to prevent unnecessary export in gyp builds.
 * Scons passes optiond -D VALIDATOR_EXPORT=DLL_EXPORT.
 * See https://code.google.com/p/nativeclient/issues/detail?id=3367 for details.
 */
#ifndef VALIDATOR_EXPORT
#define VALIDATOR_EXPORT
#endif


/* All supported features are enabled */
const NaClCPUFeaturesX86 kFullCPUIDFeatures = {
  {
    1, /* NaClCPUFeatureX86_CPUIDSupported */
    1, /* NaClCPUFeatureX86_CPUSupported */
    1, /* NaClCPUFeatureX86_3DNOW */  /* AMD-specific */
    1, /* NaClCPUFeatureX86_AES */
    1, /* NaClCPUFeatureX86_AVX */
    1, /* NaClCPUFeatureX86_AVX2 */
    1, /* NaClCPUFeatureX86_BMI1 */
    1, /* NaClCPUFeatureX86_CLFLUSH */
    1, /* NaClCPUFeatureX86_CLMUL */
    1, /* NaClCPUFeatureX86_CMOV */
    1, /* NaClCPUFeatureX86_CX16 */
    1, /* NaClCPUFeatureX86_CX8 */
    1, /* NaClCPUFeatureX86_E3DNOW */ /* AMD-specific */
    1, /* NaClCPUFeatureX86_EMMX */   /* AMD-specific */
    1, /* NaClCPUFeatureX86_F16C */
    1, /* NaClCPUFeatureX86_FMA */
    1, /* NaClCPUFeatureX86_FMA4 */ /* AMD-specific */
    1, /* NaClCPUFeatureX86_FXSR */
    1, /* NaClCPUFeatureX86_LAHF */
    1, /* NaClCPUFeatureX86_LM */
    1, /* NaClCPUFeatureX86_LWP */ /* AMD-specific */
    1, /* NaClCPUFeatureX86_LZCNT */  /* AMD-specific */
    1, /* NaClCPUFeatureX86_MMX */
    1, /* NaClCPUFeatureX86_MON */
    1, /* NaClCPUFeatureX86_MOVBE */
    1, /* NaClCPUFeatureX86_OSXSAVE */
    1, /* NaClCPUFeatureX86_POPCNT */
    1, /* NaClCPUFeatureX86_PRE */ /* AMD-specific */
    1, /* NaClCPUFeatureX86_SSE */
    1, /* NaClCPUFeatureX86_SSE2 */
    1, /* NaClCPUFeatureX86_SSE3 */
    1, /* NaClCPUFeatureX86_SSE41 */
    1, /* NaClCPUFeatureX86_SSE42 */
    1, /* NaClCPUFeatureX86_SSE4A */  /* AMD-specific */
    1, /* NaClCPUFeatureX86_SSSE3 */
    1, /* NaClCPUFeatureX86_TBM */ /* AMD-specific */
    1, /* NaClCPUFeatureX86_TSC */
    1, /* NaClCPUFeatureX86_x87 */
    1  /* NaClCPUFeatureX86_XOP */ /* AMD-specific */
  }
};

VALIDATOR_EXPORT
const NaClCPUFeaturesX86* GetFullCPUIDFeatures(void) {
    return &kFullCPUIDFeatures;
}
