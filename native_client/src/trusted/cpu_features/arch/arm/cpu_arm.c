/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/cpu_features/arch/arm/cpu_arm.h"


void NaClSetAllCPUFeaturesArm(NaClCPUFeatures *f) {
  /* TODO(jfb) Use a safe cast in this interface. */
  NaClCPUFeaturesArm *features = (NaClCPUFeaturesArm *) f;
  /* Pedantic: avoid using memset, as in x86's cpu_x86.c. */
  int id;
  /* Ensure any padding is zeroed. */
  NaClClearCPUFeaturesArm(features);
  for (id = 0; id < NaClCPUFeatureArm_Max; ++id) {
    NaClSetCPUFeatureArm(features, id, 1);
  }
}

void NaClGetCurrentCPUFeaturesArm(NaClCPUFeatures *f) {
  /* TODO(jfb) Use a safe cast in this interface. */
  NaClCPUFeaturesArm *features = (NaClCPUFeaturesArm *) f;
  /*
   * TODO(jfb) Create a whitelist of CPUs that don't leak information when
   *           TST+LDR and TST+STR are used. Disallow all for now.
   */
  NaClSetCPUFeatureArm(features, NaClCPUFeatureArm_CanUseTstMem, 0);
}

void NaClSetCPUFeatureArm(NaClCPUFeaturesArm *f, NaClCPUFeatureArmID id,
                          int state) {
  f->data[id] = (char) state;
}

const char *NaClGetCPUFeatureArmName(NaClCPUFeatureArmID id) {
  static const char *kFeatureArmNames[NaClCPUFeatureArm_Max] = {
# define NACL_ARM_CPU_FEATURE(name) NACL_TO_STRING(name),
# include "native_client/src/trusted/cpu_features/arch/arm/cpu_arm_features.h"
# undef NACL_ARM_CPU_FEATURE
  };
  return ((unsigned)id < NaClCPUFeatureArm_Max) ?
      kFeatureArmNames[id] : "INVALID";
}

void NaClClearCPUFeaturesArm(NaClCPUFeaturesArm *features) {
  memset(features, 0, sizeof(*features));
}

void NaClCopyCPUFeaturesArm(NaClCPUFeaturesArm *target,
                            const NaClCPUFeaturesArm *source) {
  memcpy(target, source, sizeof(*target));
}
