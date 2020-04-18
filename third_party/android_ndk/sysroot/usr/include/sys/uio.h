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

#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <linux/uio.h>

__BEGIN_DECLS

ssize_t readv(int __fd, const struct iovec* __iov, int __count);
ssize_t writev(int __fd, const struct iovec* __iov, int __count);

#if defined(__USE_GNU)

#if __ANDROID_API__ >= 24
ssize_t preadv(int __fd, const struct iovec* __iov, int __count, off_t __offset) __RENAME_IF_FILE_OFFSET64(preadv64) __INTRODUCED_IN(24);
ssize_t pwritev(int __fd, const struct iovec* __iov, int __count, off_t __offset) __RENAME_IF_FILE_OFFSET64(pwritev64) __INTRODUCED_IN(24);
ssize_t preadv64(int __fd, const struct iovec* __iov, int __count, off64_t __offset) __INTRODUCED_IN(24);
ssize_t pwritev64(int __fd, const struct iovec* __iov, int __count, off64_t __offset) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

#endif

#if defined(__USE_GNU)

#if __ANDROID_API__ >= 23
ssize_t process_vm_readv(pid_t __pid, const struct iovec* __local_iov, unsigned long __local_iov_count, const struct iovec* __remote_iov, unsigned long __remote_iov_count, unsigned long __flags) __INTRODUCED_IN(23);
ssize_t process_vm_writev(pid_t __pid, const struct iovec* __local_iov, unsigned long __local_iov_count, const struct iovec* __remote_iov, unsigned long __remote_iov_count, unsigned long __flags) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */

#endif

__END_DECLS

#endif
