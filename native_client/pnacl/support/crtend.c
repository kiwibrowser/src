/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Native crtend.o
 *
 * Exception handling frames are aggregated into a single section called
 * .eh_frame.  The runtime system needs to (1) have a symbol for the beginning
 * of this section, and needs to (2) mark the end of the section by a NULL.
 */

static void *__EH_FRAME_END__[]
    __attribute__((used, section(".eh_frame"), aligned(4)))
    = { (void*)0 };
