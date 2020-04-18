/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_IRT_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_IRT_H_

#include <errno.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"

extern TYPE_nacl_irt_query __nacl_irt_query;

/* These declarations are defined within libnacl. */
extern struct nacl_irt_basic __libnacl_irt_basic;
extern struct nacl_irt_fdio __libnacl_irt_fdio;
extern struct nacl_irt_dev_fdio __libnacl_irt_dev_fdio;
extern struct nacl_irt_dev_filename __libnacl_irt_dev_filename;
extern struct nacl_irt_memory __libnacl_irt_memory;
extern struct nacl_irt_tls __libnacl_irt_tls;
extern struct nacl_irt_clock __libnacl_irt_clock;
extern struct nacl_irt_dev_getpid __libnacl_irt_dev_getpid;

/* These declarations are defined within libpthread. */
extern struct nacl_irt_thread __libnacl_irt_thread;
extern struct nacl_irt_futex __libnacl_irt_futex;

extern int __libnacl_irt_query(const char *interface,
                               void *table, size_t table_size);
extern void __libnacl_mandatory_irt_query(const char *interface_ident,
                                          void *table, size_t table_size);
extern void __libnacl_irt_init(Elf32_auxv_t *auxv);

extern void __libnacl_irt_clock_init(void);
extern void __libnacl_irt_dev_fdio_init(void);
extern void __libnacl_irt_dev_filename_init(void);

/*
 * __libnacl_irt_init_fn() is used for initializing an IRT interface
 * struct on demand.  Example usage:
 *
 *   int foo_func(int arg) {
 *     if (!__libnacl_irt_init_fn(&__libnacl_irt_foo.foo_func,
 *                                __libnacl_irt_foo_init)) {
 *       return -1;
 *     }
 *     int error = __libnacl_irt_foo.foo_func(arg);
 *     if (error) {
 *       errno = error;
 *       return -1;
 *     }
 *     return 0;
 *   }
 *
 * This pattern assumes that the IRT's query function writes each
 * function pointer to the interface struct atomically, so that this
 * is thread-safe if foo_func() is called in multiple threads.
 *
 * A limitation of this approach is that, if the IRT does not provide
 * the "foo" interface, each call to foo_func() will call the IRT's
 * query function.
 */
extern int __libnacl_irt_init_fn(void *interface_field, void (*init)(void));

#endif  /* NATIVE_CLIENT_SRC_UNTRUSTED_NACL_IRT_H_ */
