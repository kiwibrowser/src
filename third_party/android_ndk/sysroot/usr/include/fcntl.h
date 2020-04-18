/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _FCNTL_H
#define _FCNTL_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <linux/fadvise.h>
#include <linux/falloc.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/uio.h>

#include <bits/fcntl.h>
#include <bits/seek_constants.h>

#if defined(__USE_GNU) || defined(__USE_BSD)
#include <bits/lockf.h>
#endif

__BEGIN_DECLS

#ifdef __LP64__
/* LP64 kernels don't have F_*64 defines because their flock is 64-bit. */
#define F_GETLK64  F_GETLK
#define F_SETLK64  F_SETLK
#define F_SETLKW64 F_SETLKW
#endif

#define O_ASYNC FASYNC
#define O_RSYNC O_SYNC

#if __ANDROID_API__ >= __ANDROID_API_L__
#define SPLICE_F_MOVE 1
#define SPLICE_F_NONBLOCK 2
#define SPLICE_F_MORE 4
#define SPLICE_F_GIFT 8
#endif

#if __ANDROID_API__ >= __ANDROID_API_O__
#define SYNC_FILE_RANGE_WAIT_BEFORE 1
#define SYNC_FILE_RANGE_WRITE 2
#define SYNC_FILE_RANGE_WAIT_AFTER 4
#endif

int creat(const char* __path, mode_t __mode);

#if __ANDROID_API__ >= 21
int creat64(const char* __path, mode_t __mode) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

int openat(int __dir_fd, const char* __path, int __flags, ...) __overloadable __RENAME_CLANG(openat);

#if __ANDROID_API__ >= 21
int openat64(int __dir_fd, const char* __path, int __flags, ...) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

int open(const char* __path, int __flags, ...) __overloadable __RENAME_CLANG(open);

#if __ANDROID_API__ >= 21
int open64(const char* __path, int __flags, ...) __INTRODUCED_IN(21);
ssize_t splice(int __in_fd, off64_t* __in_offset, int __out_fd, off64_t* __out_offset, size_t __length, unsigned int __flags) __INTRODUCED_IN(21);
ssize_t tee(int __in_fd, int __out_fd, size_t __length, unsigned int __flags) __INTRODUCED_IN(21);
ssize_t vmsplice(int __fd, const struct iovec* __iov, size_t __count, unsigned int __flags) __INTRODUCED_IN(21);

int fallocate(int __fd, int __mode, off_t __offset, off_t __length) __RENAME_IF_FILE_OFFSET64(fallocate64) __INTRODUCED_IN(21);
int fallocate64(int __fd, int __mode, off64_t __offset, off64_t __length) __INTRODUCED_IN(21);
int posix_fadvise(int __fd, off_t __offset, off_t __length, int __advice) __RENAME_IF_FILE_OFFSET64(posix_fadvise64) __INTRODUCED_IN(21);
int posix_fadvise64(int __fd, off64_t __offset, off64_t __length, int __advice) __INTRODUCED_IN(21);
int posix_fallocate(int __fd, off_t __offset, off_t __length) __RENAME_IF_FILE_OFFSET64(posix_fallocate64) __INTRODUCED_IN(21);
int posix_fallocate64(int __fd, off64_t __offset, off64_t __length) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */


#if defined(__USE_GNU)

#if __ANDROID_API__ >= 16
ssize_t readahead(int __fd, off64_t __offset, size_t __length) __INTRODUCED_IN(16);
#endif /* __ANDROID_API__ >= 16 */


#if __ANDROID_API__ >= 26
int sync_file_range(int __fd, off64_t __offset, off64_t __length, unsigned int __flags) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */

#endif

#if defined(__BIONIC_INCLUDE_FORTIFY_HEADERS)
#include <bits/fortify/fcntl.h>
#endif

__END_DECLS

#endif
