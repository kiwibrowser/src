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

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <sys/cdefs.h>

__BEGIN_DECLS

struct timespec;

typedef struct {
  unsigned int count;
#ifdef __LP64__
  int __reserved[3];
#endif
} sem_t;

#define SEM_FAILED __BIONIC_CAST(reinterpret_cast, sem_t*, 0)

int sem_destroy(sem_t* __sem);
int sem_getvalue(sem_t* __sem, int* __value);
int sem_init(sem_t* __sem, int __shared, unsigned int __value);
int sem_post(sem_t* __sem);
int sem_timedwait(sem_t* __sem, const struct timespec* __ts);
int sem_trywait(sem_t* __sem);
int sem_wait(sem_t* __sem);

/* These aren't actually implemented. */
sem_t* sem_open(const char* __name, int _flags, ...);
int sem_close(sem_t* __sem);
int sem_unlink(const char* __name);

__END_DECLS

#endif
