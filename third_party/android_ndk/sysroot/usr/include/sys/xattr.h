/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef _SYS_XATTR_H_
#define _SYS_XATTR_H_

#include <linux/xattr.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS


#if __ANDROID_API__ >= 16
int fsetxattr(int __fd, const char* __name, const void* __value, size_t __size, int __flags)
  __INTRODUCED_IN(16);
int setxattr(const char* __path, const char* __name, const void* __value, size_t __size, int __flags)
  __INTRODUCED_IN(16);
int lsetxattr(const char* __path, const char* __name, const void* __value, size_t __size, int __flags)
  __INTRODUCED_IN(16);

ssize_t fgetxattr(int __fd, const char* __name, void* __value, size_t __size) __INTRODUCED_IN(16);
ssize_t getxattr(const char* __path, const char* __name, void* __value, size_t __size) __INTRODUCED_IN(16);
ssize_t lgetxattr(const char* __path, const char* __name, void* __value, size_t __size) __INTRODUCED_IN(16);

ssize_t listxattr(const char* __path, char* __list, size_t __size) __INTRODUCED_IN(16);
ssize_t llistxattr(const char* __path, char* __list, size_t __size) __INTRODUCED_IN(16);
ssize_t flistxattr(int __fd, char* __list, size_t __size) __INTRODUCED_IN(16);

int removexattr(const char* __path, const char* __name) __INTRODUCED_IN(16);
int lremovexattr(const char* __path, const char* __name) __INTRODUCED_IN(16);
int fremovexattr(int __fd, const char* __name) __INTRODUCED_IN(16);
#endif /* __ANDROID_API__ >= 16 */


__END_DECLS

#endif
