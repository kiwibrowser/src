/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_NONSFI_EXAMPLE_INTERFACE_H_
#define NATIVE_CLIENT_TESTS_NONSFI_EXAMPLE_INTERFACE_H_ 1

#include <stdint.h>

#define NACL_IRT_EXAMPLE_v1_0 "nacl-irt-example-1.0"

/* Example IRT interface. */
struct nacl_irt_example {
  int (*add_one)(uint32_t number, uint32_t *result);
  /* More functions can go here. */
};

#endif
