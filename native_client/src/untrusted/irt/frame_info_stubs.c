/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* On x86-64 we build the IRT with sandbox base-address hiding. This means that
 * all of its components must be built with LLVM's assembler. Currently the
 * unwinder library is built with nacl-gcc and so does not use base address
 * hiding. The IRT does not use exceptions, so we do not actually need the
 * unwinder at all. To prevent it from being linked into the IRT, we provide
 * stub implementations of its functions that are referenced from crtbegin.c.
 */

void __register_frame_info(void *begin, void *ob) {}
void __deregister_frame_info(const void *begin) {}
