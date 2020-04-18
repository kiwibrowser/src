/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_IRT_CORE_H_
#define NATIVE_CLIENT_SRC_PUBLIC_IRT_CORE_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * This header file declares functions that may be used by an embedder
 * of NaCl (such as Chromium) to implement a NaCl IRT.
 */

#include <stddef.h>
#include <stdint.h>

/* Type of the IRT query function provided by the IRT to user code. */
typedef size_t (*nacl_irt_query_func_t)(const char *interface_ident,
                                        void *table, size_t tablesize);

/*
 * nacl_irt_start() is the entry point that the embedder should
 * define.  This is similar to _start(), except that _start() is
 * defined by the standard libraries (specifically, by libnacl) and an
 * IRT cannot reliably override _start() by defining it in a ".a"
 * library.  nacl_irt_start() may, however, be defined by a ".a"
 * library.
 *
 * See nacl_startup.h for the layout of the |info| pointer.
 */
void nacl_irt_start(uint32_t *info);

/*
 * nacl_irt_init() initializes libc and Thread Local Storage (TLS
 * variables) inside the IRT.  Calling nacl_irt_init() will typically
 * be the first thing that nacl_irt_start() does.
 *
 * This is provided as a separate function just in case there is any
 * initialization the IRT needs to do before nacl_irt_init(), or for
 * very minimal IRTs that do not need to initialize libc.
 */
void nacl_irt_init(uint32_t *info);

/*
 * nacl_irt_enter_user_code() jumps to the entry point of the user
 * nexe, passing |query_func| to it via an AT_SYSINFO entry in the
 * auxiliary vector.  This function does not return.
 */
void nacl_irt_enter_user_code(uint32_t *info, nacl_irt_query_func_t query_func);

/* Function type for user code's initial entry point. */
typedef void (*nacl_entry_func_t)(uint32_t *args);

/*
 * nacl_irt_nonsfi_entry() acts like nacl_irt_enter_user_code,
 * but allows passing in additional arguments.
 */
int nacl_irt_nonsfi_entry(int argc, char **argv, char **environ,
                          nacl_entry_func_t entry_func,
                          nacl_irt_query_func_t query_func);

/* For nonsfi_loader. */
void nacl_irt_nonsfi_allow_dev_interfaces(void);

/* Function for querying for NaCl's core IRT interfaces. */
size_t nacl_irt_query_core(const char *interface_ident,
                           void *table, size_t tablesize);

/* Definition of an IRT interface, for use with nacl_irt_query_list(). */
struct nacl_irt_interface {
  const char *name;
  const void *table;
  size_t size;
  /*
   * filter() returns whether the interface should be enabled.
   * |filter| may be NULL, in which case the interface is always
   * enabled.
   */
  int (*filter)(void);
};

/*
 * nacl_irt_query_list() is a helper function for defining an IRT
 * query function given an array of nacl_irt_interface structs (as
 * specified by |available| and |available_size|, which is the size of
 * the array in bytes rather than the number of entries).
 */
size_t nacl_irt_query_list(const char *interface_ident,
                           void *table, size_t tablesize,
                           const struct nacl_irt_interface *available,
                           size_t available_size);

EXTERN_C_END

#endif
