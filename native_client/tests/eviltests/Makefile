# Copyright 2008 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# a simple example of a Linux/MacOS program that makes a system call directly
trustme: trustme.c
	gcc -o trustme trustme.c

# Build this way works with MacOS 10.5 and gcc 4.0
macos_overlapjmp: macos_overlapjmp.c
	gcc -O0 -o $@ $<

# Build as follows for gcc 4.0.3 on Ubuntu
linux_overlapjmp: linux_overlapjmp.c
	gcc -O0 -o $@ $<

vmdefect: vmdefect.s
	as -o vmdefect.o vmdefect.s
	ld -o vmdefect vmdefect.o

clean:
	rm trustme linux_overlapjmp macos_overlapjmp
