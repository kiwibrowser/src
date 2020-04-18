/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * This generates a full page of halt instructions.
 * All that matters for this test is that it's all valid instructions.
 */
.text
.fill (1 << NACL_MAP_PAGESHIFT) / NACL_HALT_LEN, NACL_HALT_LEN, NACL_HALT_OPCODE
