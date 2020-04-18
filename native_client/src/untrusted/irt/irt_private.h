/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_PRIVATE_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_PRIVATE_H_

#include <stdint.h>

extern __thread int g_is_irt_internal_thread;

extern uintptr_t g_dynamic_text_start;

void irt_reserve_code_allocation(uintptr_t code_begin, size_t code_size);

#endif  /* NATIVE_CLIENT_SRC_UNTRUSTED_IRT_IRT_PRIVATE_H_ */
