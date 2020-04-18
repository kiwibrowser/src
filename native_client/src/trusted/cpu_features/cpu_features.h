/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_CPU_FEATURES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_CPU_FEATURES_H_

#include "native_client/src/include/nacl_base.h"


EXTERN_C_BEGIN

/*
 * Forward-declared (but never defined) generic CPU features.
 * Each architecture needs to cast from this generic type.
 */
struct NaClCPUFeaturesAbstract;
typedef struct NaClCPUFeaturesAbstract NaClCPUFeatures;

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_CPU_FEATURES_CPU_FEATURES_H_ */
