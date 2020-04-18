/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

extern int module_a_var;

int module_b_var = 1234;

int *addr_of_module_a_var = &module_a_var;
