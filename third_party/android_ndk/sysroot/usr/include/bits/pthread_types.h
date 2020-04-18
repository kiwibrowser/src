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

#ifndef _BITS_PTHREAD_TYPES_H_
#define _BITS_PTHREAD_TYPES_H_

#include <sys/cdefs.h>
#include <sys/types.h>

typedef struct {
  uint32_t flags;
  void* stack_base;
  size_t stack_size;
  size_t guard_size;
  int32_t sched_policy;
  int32_t sched_priority;
#ifdef __LP64__
  char __reserved[16];
#endif
} pthread_attr_t;

#if __ANDROID_API__ >= __ANDROID_API_N__
typedef struct {
#if defined(__LP64__)
  int64_t __private[4];
#else
  int32_t __private[8];
#endif
} pthread_barrier_t;
#endif

#if __ANDROID_API__ >= __ANDROID_API_N__
typedef int pthread_barrierattr_t;
#endif

typedef struct {
#if defined(__LP64__)
  int32_t __private[12];
#else
  int32_t __private[1];
#endif
} pthread_cond_t;

typedef long pthread_condattr_t;

typedef int pthread_key_t;

typedef struct {
#if defined(__LP64__)
  int32_t __private[10];
#else
  int32_t __private[1];
#endif
} pthread_mutex_t;

typedef long pthread_mutexattr_t;

typedef int pthread_once_t;

typedef struct {
#if defined(__LP64__)
  int32_t __private[14];
#else
  int32_t __private[10];
#endif
} pthread_rwlock_t;

typedef long pthread_rwlockattr_t;

#if __ANDROID_API__ >= __ANDROID_API_N__
typedef struct {
#if defined(__LP64__)
  int64_t __private;
#else
  int32_t __private[2];
#endif
} pthread_spinlock_t;
#endif

typedef long pthread_t;

#endif
