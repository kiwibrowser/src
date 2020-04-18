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

#ifndef _SYS_INOTIFY_H_
#define _SYS_INOTIFY_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>
#include <linux/inotify.h>
#include <asm/fcntl.h> /* For O_CLOEXEC and O_NONBLOCK. */

__BEGIN_DECLS

/*
 * Some third-party code uses the existence of IN_CLOEXEC/IN_NONBLOCK to detect
 * the availability of inotify_init1. This is not correct, since
 * `syscall(__NR_inotify_init1, IN_CLOEXEC)` is still valid even if the C
 * library doesn't have that function, but for the time being we don't want to
 * harm adoption to the unified headers. We'll avoid defining IN_CLOEXEC and
 * IN_NONBLOCK if we don't have inotify_init1 for the time being, and maybe
 * revisit this later.
 *
 * https://github.com/android-ndk/ndk/issues/394
 */
#if __ANDROID_API__ >= __ANDROID_API_L__
#define IN_CLOEXEC O_CLOEXEC
#define IN_NONBLOCK O_NONBLOCK
#endif

int inotify_init(void);

#if __ANDROID_API__ >= 21
int inotify_init1(int __flags) __INTRODUCED_IN(21);
#endif /* __ANDROID_API__ >= 21 */

int inotify_add_watch(int __fd, const char* __path, uint32_t __mask);
int inotify_rm_watch(int __fd, uint32_t __watch_descriptor);

__END_DECLS

#endif
