/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error("This file is not meant for use in the TCB")
#endif

/*
 * cpu_x86_test.c
 * test main and subroutines for cpu_x86
 */
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include <stdio.h>
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"

int main(void) {
  NaClCPUFeaturesX86 fv;
  NaClCPUData cpu_data;
  int feature_id;
  NaClCPUDataGet(&cpu_data);

  NaClGetCurrentCPUFeaturesX86((NaClCPUFeatures *) &fv);
  if (NaClArchSupportedX86(&fv)) {
    printf("This is a native client %d-bit %s compatible computer\n",
           NACL_BUILD_SUBARCH, GetCPUIDString(&cpu_data));
  } else {
    if (!NaClGetCPUFeatureX86(&fv, NaClCPUFeatureX86_CPUIDSupported)) {
      printf("Computer doesn't support CPUID\n");
    }
    if (!NaClGetCPUFeatureX86(&fv, NaClCPUFeatureX86_CPUSupported)) {
      printf("Computer id %s is not supported\n", GetCPUIDString(&cpu_data));
    }
  }

  printf("This processor has:\n");
  for (feature_id = 0; feature_id < NaClCPUFeatureX86_Max; ++feature_id) {
    if (NaClGetCPUFeatureX86(&fv, feature_id))
      printf("        %s\n", NaClGetCPUFeatureX86Name(feature_id));
  }
  return 0;
}
