/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_ARCH_MIPS_CPU_MIPS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_ARCH_MIPS_CPU_MIPS_H_

#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator/ncvalidate.h"


EXTERN_C_BEGIN

/* List of features supported by MIPS CPUs. */
typedef enum {
#define NACL_MIPS_CPU_FEATURE(name) NACL_CONCAT(NaClCPUFeatureMips_, name),
#include "native_client/src/trusted/cpu_features/arch/mips/cpu_mips_features.h"
#undef NACL_MIPS_CPU_FEATURE
  /* Leave the following as the last entry. */
  NaClCPUFeatureMips_Max
} NaClCPUFeatureMipsID;

typedef struct cpu_feature_struct_mips {
  char data[NaClCPUFeatureMips_Max];
} NaClCPUFeaturesMips;

/*
 * Platform-independent NaClValidatorInterface functions.
 */
void NaClSetAllCPUFeaturesMips(NaClCPUFeatures *features);
void NaClGetCurrentCPUFeaturesMips(NaClCPUFeatures *features);

/*
 * Platform-dependent getter/setter.
 */
static INLINE int NaClGetCPUFeatureMips(const NaClCPUFeaturesMips *features,
                                        NaClCPUFeatureMipsID id) {
  return features->data[id];
}

void NaClSetCPUFeatureMips(NaClCPUFeaturesMips *features,
                           NaClCPUFeatureMipsID id, int state);
const char *NaClGetCPUFeatureMipsName(NaClCPUFeatureMipsID id);

/*
 * Platform-independent functions which are only used in platform-dependent
 * code.
 */
void NaClClearCPUFeaturesMips(NaClCPUFeaturesMips *features);
/*
 * TODO(jfb) The x86 CPU features also offers these functions, which are
 *  currently notused on MIPS: NaClCopyCPUFeaturesMips, NaClArchSupportedMips.
 */

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_ARCH_MIPS_CPU_MIPS_H_
