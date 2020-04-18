/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

    /*
     * This routine implements a Linux/ELF entry point, where argc/argv and
     * the environment are passed on the stack.  This startup protocol is
     * not compatible with normal C function calling conventions.  This
     * routine converts to C calling conventions by passing the stack
     * pointer to __pnacl_start_linux_c().
     *
     * The ELF ABI says that r0, if non-zero, points to a finalization
     * function (for registering with atexit()).  That is only set by the
     * dynamic linker, not by the kernel, so we ignore it here.
     */
    .global __pnacl_start_linux
    .type __pnacl_start_linux, #function
__pnacl_start_linux:
    /*
     * Zero the frame pointer to make the outermost frame more obvious to
     * debuggers.  This is probably not necessary because the kernel
     * probably always does this, but it doesn't hurt to do it.
     */
    mov fp, #0

    mov r0, sp  /* Set up argument */
    bl __pnacl_start_linux_c
    bkpt 0
