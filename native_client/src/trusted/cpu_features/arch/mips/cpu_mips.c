/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/cpu_features/arch/mips/cpu_mips.h"


void NaClSetAllCPUFeaturesMips(NaClCPUFeatures *f) {
  /* TODO(jfb) Use a safe cast in this interface. */
  NaClCPUFeaturesMips *features = (NaClCPUFeaturesMips *) f;
  /* Pedantic: avoid using memset, as in x86's cpu_x86.c. */
  int id;
  /* Ensure any padding is zeroed. */
  NaClClearCPUFeaturesMips(features);
  for (id = 0; id < NaClCPUFeatureMips_Max; ++id) {
    NaClSetCPUFeatureMips(features, id, 1);
  }
}

void NaClGetCurrentCPUFeaturesMips(NaClCPUFeatures *f) {
  /* TODO(jfb) Use a safe cast in this interface. */
  NaClCPUFeaturesMips *features = (NaClCPUFeaturesMips *) f;
  features->data[NaClCPUFeatureMips_BOGUS] = 0;
}

void NaClSetCPUFeatureMips(NaClCPUFeaturesMips *f, NaClCPUFeatureMipsID id,
                           int state) {
  f->data[id] = (char) state;
}

const char *NaClGetCPUFeatureMipsName(NaClCPUFeatureMipsID id) {
  static const char *kFeatureMipsNames[NaClCPUFeatureMips_Max] = {
# define NACL_MIPS_CPU_FEATURE(name) NACL_TO_STRING(name),
# include "native_client/src/trusted/cpu_features/arch/mips/cpu_mips_features.h"
# undef NACL_MIPS_CPU_FEATURE
  };
  return ((unsigned)id < NaClCPUFeatureMips_Max) ?
      kFeatureMipsNames[id] : "INVALID";
}

void NaClClearCPUFeaturesMips(NaClCPUFeaturesMips *features) {
  memset(features, 0, sizeof(*features));
}
