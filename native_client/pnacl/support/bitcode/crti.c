/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * PNaCl handles init/fini through the .init_array and .fini_array sections
 * rather than .init/.fini.  We provide stubs here because they are still
 * called from newlib's startup routine in libc/misc/init.c and
 * glibc's start routine. (csu/elf-init.c)
 */
void _init(void) { }
void _fini(void) { }
