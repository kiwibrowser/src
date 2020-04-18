/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This is a workaround for service_runtime/include/machine/_types.h,
 * which wants to define dev_t, ino_t and others, but these are normally
 * defined by glibc.  However, we need machine/_types.h for its
 * definition of nacl_abi_size_t.
 *
 * To resolve this, we #include glibc headers that cause __*_t_defined
 * macros to be defined, which tells machine/_types.h not to attempt to
 * define these types again.
 *
 * TODO(mseaborn): We should remove _default_types.h because it is a
 * newlibism.  However, this is tricky because of the magic rewriting of
 * the "NACL_ABI_" and "nacl_abi_" prefixes that the NaCl headers use.
 */

/* This gives us __dev_t_defined, __ino_t_defined and others */
#include <sys/types.h>
/* This gives us __time_t_defined */
#include <time.h>
