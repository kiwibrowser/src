/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

.global _start
.p2align 5 # Set up a 32 byte boundary
nop # Add misalignment -- should fail a test which checks for this
_start:
jmp _start
