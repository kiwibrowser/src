/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This module provides a simple abstraction for using the CPUID
 * instruction to determine instruction set extensions supported by
 * the current processor.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_ARCH_X86_CPU_X86_H_
#define NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_ARCH_X86_CPU_X86_H_

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator/ncvalidate.h"


EXTERN_C_BEGIN

/* List of features supported by x86 CPUs. */
typedef enum {
#define NACL_X86_CPU_FEATURE(id, reg, idx, ven, str)       \
  NACL_CONCAT(NaClCPUFeatureX86_, id),
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86_features.h"
#undef NACL_X86_CPU_FEATURE
  /* Leave the following as the last entry. */
  NaClCPUFeatureX86_Max
} NaClCPUFeatureX86ID;

/* Features we can get about the x86 hardware. */
typedef struct cpu_feature_struct_X86 {
  char data[NaClCPUFeatureX86_Max];
} NaClCPUFeaturesX86;

/* Define the maximum length of a CPUID string.
 *
 * Note: If you change this length, fix the static initialization of wlid
 * in cpu_x86.c to be initialized with an appropriate string.
 */
#define /* static const int */ kCPUIDStringLength 21

/* Defines the maximum number of feature registers used to hold CPUID.
 * Note: This value corresponds to the number of enumerated elements in
 * enum CPUFeatureReg defined in cpu_x86.c.
 */
#define kMaxCPUFeatureReg 12

/* Defines the maximum number of extended control registers.
 */
#define kMaxCPUXCRReg 1

/* Define a cache for collected CPU runtime information, from which
 * queries can answer questions.
 */
typedef struct NaClCPUData {
  /* The following is used to cache whether CPUID is defined for the
   * architecture the code is running on.
   */
  int _has_CPUID;
  /* Version ID words used by CPUVersionID. */
  uint32_t _vidwords[4];
  /* Define the set of CPUID feature register values for the architecture.
   * Note: We have two sets (of 4 registers) so that AMD specific flags can be
   * picked up.
   */
  uint32_t _featurev[kMaxCPUFeatureReg];
  /* Define the set of extended control register (XCR) values.
   */
  uint64_t _xcrv[kMaxCPUXCRReg];
  /* Define a string to hold and cache the CPUID. In such cases, such races
   * will at worst cause the CPUID to not be recognized.
   */
  char _wlid[kCPUIDStringLength];
} NaClCPUData;

/* Collect CPU data about this CPU, and put into the given data structure.
 */
void NaClCPUDataGet(NaClCPUData* data);

/* GetCPUIDString creates an ASCII string that identifies this CPU's
 * vendor ID, family, model, and stepping, as per the CPUID instruction
 */
char *GetCPUIDString(NaClCPUData* data);

/*
 * Platform-independent NaClValidatorInterface functions.
 */
void NaClSetAllCPUFeaturesX86(NaClCPUFeatures *features);
void NaClGetCurrentCPUFeaturesX86(NaClCPUFeatures *cpu_features);

/*
 * Platform-dependent getter/setter.
 */
static INLINE int NaClGetCPUFeatureX86(const NaClCPUFeaturesX86 *features,
                                       NaClCPUFeatureX86ID id) {
  return features->data[id];
}

void NaClSetCPUFeatureX86(NaClCPUFeaturesX86 *features, NaClCPUFeatureX86ID id,
                          int state);
const char *NaClGetCPUFeatureX86Name(NaClCPUFeatureX86ID id);

/*
 * Platform-independent functions which are only used in platform-dependent
 * code.
 * TODO(jfb) The ARM and MIPS CPU feature do not offer NaClCopyCPUFeaturesX86
 * and NaClArchSupportedX86, should they be removed?
 */
void NaClClearCPUFeaturesX86(NaClCPUFeaturesX86 *features);
void NaClCopyCPUFeaturesX86(NaClCPUFeaturesX86 *target,
                            const NaClCPUFeaturesX86 *source);
int NaClArchSupportedX86(const NaClCPUFeaturesX86 *features);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_ARCH_X86_CPU_X86_H_ */
