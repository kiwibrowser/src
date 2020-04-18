/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef _SYS_MSG_H_
#define _SYS_MSG_H_

#include <sys/cdefs.h>
#include <sys/ipc.h>

#include <linux/msg.h>

#define msqid_ds msqid64_ds

__BEGIN_DECLS

typedef __kernel_ulong_t msgqnum_t;
typedef __kernel_ulong_t msglen_t;


#if __ANDROID_API__ >= 26
int msgctl(int __msg_id, int __cmd, struct msqid_ds* __buf) __INTRODUCED_IN(26);
int msgget(key_t __key, int __flags) __INTRODUCED_IN(26);
ssize_t msgrcv(int __msg_id, void* __msgbuf_ptr, size_t __size, long __type, int __flags) __INTRODUCED_IN(26);
int msgsnd(int __msg_id, const void* __msgbuf_ptr, size_t __size, int __flags) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */


__END_DECLS

#endif
