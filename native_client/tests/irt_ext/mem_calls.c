/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_extension.h"
#include "native_client/tests/irt_ext/error_report.h"
#include "native_client/tests/irt_ext/mem_calls.h"

static struct mem_calls_environment *g_activated_env = NULL;
static const struct mem_calls_environment g_empty_env = { 0 };
static struct nacl_irt_memory g_irt_memory = { NULL };

static int my_mmap(void **addr, size_t len, int prot, int flags, int fd,
                   nacl_irt_off_t off) {
  if (g_activated_env) {
    g_activated_env->num_mmap_calls++;
  }

  return g_irt_memory.mmap(addr, len, prot, flags, fd, off);
}

static int my_munmap(void *addr, size_t len) {
  if (g_activated_env) {
    g_activated_env->num_munmap_calls++;
  }

  return g_irt_memory.munmap(addr, len);
}

static int my_mprotect(void *addr, size_t len, int prot) {
  if (g_activated_env) {
    g_activated_env->num_mprotect_calls++;
  }

  return g_irt_memory.mprotect(addr, len, prot);
}

void init_mem_calls_module(void) {
  size_t bytes = nacl_interface_query(NACL_IRT_MEMORY_v0_3,
                                      &g_irt_memory, sizeof(g_irt_memory));
  IRT_EXT_ASSERT_MSG(bytes == sizeof(g_irt_memory),
                     "Could not query interface: " NACL_IRT_MEMORY_v0_3);

  struct nacl_irt_memory mem = {
    my_mmap,
    my_munmap,
    my_mprotect
  };

  bytes = nacl_interface_ext_supply(NACL_IRT_MEMORY_v0_3,
                                    &mem, sizeof(mem));
  IRT_EXT_ASSERT_MSG(bytes == sizeof(mem),
                     "Could not supply interface: " NACL_IRT_MEMORY_v0_3);
}

void init_mem_calls_environment(struct mem_calls_environment *env) {
  *env = g_empty_env;
}

void activate_mem_calls_env(struct mem_calls_environment *env) {
  g_activated_env = env;
}

void deactivate_mem_calls_env(void) {
  g_activated_env = NULL;
}
