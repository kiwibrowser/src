// Copyright 2016 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_PLL_LOADER_PLL_ROOT_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_PLL_LOADER_PLL_ROOT_H_ 1

#include <stddef.h>

struct PLLTLSBlockGetter {
  // Returns a pointer to the module's TLS block for the current thread.
  void *(*func)(PLLTLSBlockGetter *closure);
  // The dynamic linker can use "arg" to store an ID for the module.
  void *arg;
};

struct PLLTLSVarGetter {
  // Returns the address of the TLS variable in the current thread.
  void *(*func)(struct PLLTLSVarGetter *var_getter);
  // The dynamic linker can use "arg1" to store an ID for the module.
  void *arg1;
  // The dynamic linker can use "arg2" to store an offset for the variable.
  void *arg2;
};

// This is the root data structure exported by a PLL (PNaCl/portable
// loadable library).
struct PLLRoot {
  size_t format_version;
  const char *string_table;

  // Exports.
  void *const *exported_ptrs;
  const size_t *exported_names;
  size_t export_count;

  // Imports.
  void *const *imported_ptrs;
  const size_t *imported_names;
  size_t import_count;

  // Hash Table, for quick exported symbol lookup.
  size_t bucket_count;
  const int32_t *hash_buckets;
  const uint32_t *hash_chains;

  // Bloom Filter, for quick exported symbol lookup rejection.
  size_t bloom_filter_maskwords_bitmask;
  // The number of right shifts to generate the second hash from the first.
  size_t bloom_filter_shift2;
  const uint32_t *bloom_filter_data;

  // Thread-local variables (TLS).
  void *tls_template;
  size_t tls_template_data_size;  // Size of initialized data.
  size_t tls_template_total_size;  // Size of initialized data + BSS.
  size_t tls_template_alignment;
  PLLTLSBlockGetter *tls_block_getter;

  // Dependencies list (akin to DT_NEEDED in ELF).
  size_t dependencies_count;
  // Null-separated list of library sonames.
  // For example, "libfoo.so\0libbar.so\0" would have dependencies_count = 2.
  const char *dependencies_list;

  // TLS Imports.
  PLLTLSVarGetter *imported_tls_ptrs;
  const size_t *imported_tls_names;
  size_t import_tls_count;
};

#endif
