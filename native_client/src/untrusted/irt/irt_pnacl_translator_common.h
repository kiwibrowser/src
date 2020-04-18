/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_PNACL_TRANSLATOR_COMMON_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_PNACL_TRANSLATOR_COMMON_H_

#include <stdlib.h>

char *env_var_prefix_match(char *env_str, const char *prefix);
size_t env_var_prefix_match_count(const char *prefix);

#endif
