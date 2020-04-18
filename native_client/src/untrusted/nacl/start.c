/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <unistd.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/nacl_startup.h"
#include "native_client/src/untrusted/nacl/start.h"
#include "native_client/src/untrusted/nacl/tls.h"


void __libc_init_array(void);
void __libc_fini_array(void);

/*
 * Alternative NaCl main entry point.  If this symbol name is found at
 * link time it is used in favour of 'main' as the program entry point.
 */
int __nacl_main(int argc, char **argv, char **envp) __attribute__((weak));

void *__nacl_initial_thread_stack_end;

/*
 * This is the true entry point for untrusted code.
 * See nacl_startup.h for the layout at the argument pointer.
 */
void _start(uint32_t *info) {
  void (*fini)(void) = nacl_startup_fini(info);
  int argc = nacl_startup_argc(info);
  char **argv = nacl_startup_argv(info);
  char **envp = nacl_startup_envp(info);
  Elf32_auxv_t *auxv = nacl_startup_auxv(info);

  environ = envp;

  /*
   * Record the approximate address from which the stack grows
   * (usually downwards) so that libpthread can report it.  Taking the
   * address of any stack-allocated variable will work here.
   */
  __nacl_initial_thread_stack_end = &info;

  __libnacl_irt_init(auxv);

  /*
   * If we were started by a dynamic linker, then it passed its finalizer
   * function here.  For static linking, this is always NULL.
   */
  if (fini != NULL)
    atexit(fini);

  atexit(&__libc_fini_array);

  __pthread_initialize();

  __libc_init_array();

  int (*main_ptr)(int argc, char **argv, char **envp) = &__nacl_main;
  if (main_ptr == NULL)
    main_ptr = &main;

  exit(main_ptr(argc, argv, envp));

  /*NOTREACHED*/
  __builtin_trap();
}
