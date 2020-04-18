/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * portability macros for file descriptors, read, write, open, etc.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_IO_H_
#define NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_IO_H_ 1

#include <fcntl.h>

#include "native_client/src/include/build_config.h"

#if NACL_WINDOWS
/* disable warnings for deprecated _snprintf */
#pragma warning(disable : 4996)

#include <io.h>
#include <direct.h>

#define DUP  _dup
#define DUP2 _dup2
#define OPEN _open
#define CLOSE _close
#define FDOPEN _fdopen
#define PORTABLE_DEV_NULL "nul"
#define SNPRINTF _snprintf
#define VSNPRINTF _vsnprintf
#define UNLINK _unlink
#define MKDIR(p, m) _mkdir(p)  /* BEWARE MODE BITS ARE DROPPED! */
#define RMDIR _rmdir
#define WRITE _write

/* Seek method constants */
#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0

/* missing from win stdio.h and fcntl.h */

/* from bits/fcntl.h */
#define O_ACCMODE 0003

#else

#include <stdlib.h>
#include <unistd.h>

#define OPEN open
#define CLOSE close
#define DUP  dup
#define DUP2 dup2
#define FDOPEN fdopen
#define PORTABLE_DEV_NULL  "/dev/null"
#define SNPRINTF snprintf
#define VSNPRINTF vsnprintf
#define UNLINK unlink
#define MKDIR(p, m) mkdir(p, m)
#define RMDIR rmdir
#define _O_BINARY 0
#define WRITE write
#endif

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_IO_H_ */
