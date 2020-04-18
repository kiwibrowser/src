# Copyright 2010 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Everything builds using scons and gyp, but this particular test can
# be compiled using make (and this Makefile) to generate a native
# executable on Linux and OSX.  The program is still interesting in
# that it makes it easy to discover limits of the host OS -- pthread
# library limits, system resource limits, etc.

CFLAGS=-Werror -Wall

egyptian_cotton: egyptian_cotton.o
	$(CC) -o $@ $< -lpthread
