/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This module is intended to keep track of memory calls that have been hooked
 * into a standard library. Because the IRT has a full implementation of
 * all the memory related calls (mmap, munmap, mprotect...etc.) this module
 * simply passes the actual functionality to the IRT.
 */

#ifndef NATIVE_CLIENT_TESTS_IRT_EXT_MEM_CALLS_H
#define NATIVE_CLIENT_TESTS_IRT_EXT_MEM_CALLS_H

struct mem_calls_environment {
  int num_mmap_calls;
  int num_munmap_calls;
  int num_mprotect_calls;
};

void init_mem_calls_module(void);

void init_mem_calls_environment(struct mem_calls_environment *env);

void activate_mem_calls_env(struct mem_calls_environment *env);
void deactivate_mem_calls_env(void);

#endif /* NATIVE_CLIENT_TESTS_IRT_EXT_MEM_CALLS_H */
