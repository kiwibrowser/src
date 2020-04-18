/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Native Client dummy implementation for locks - required for newlib
 */

#include <sys/unistd.h>

struct _LOCK_T;

/*
 * These functions provide dummy definitions when libpthread is not being
 * used to resolve symbols in other parts of the runtime library.  Because
 * they are marked as weak symbols, the definitions are overridden if the
 * real libpthread is used.
 */
void __local_lock_init(_LOCK_T* lock) __attribute__ ((weak));
void __local_lock_init_recursive(_LOCK_T* lock) __attribute__ ((weak));
void __local_lock_close(_LOCK_T* lock) __attribute__ ((weak));
void __local_lock_close_recursive(_LOCK_T* lock) __attribute__ ((weak));
void __local_lock_acquire(_LOCK_T* lock) __attribute__ ((weak));
void __local_lock_acquire_recursive(_LOCK_T* lock) __attribute__ ((weak));
int __local_lock_try_acquire(_LOCK_T* lock) __attribute__ ((weak));
int __local_lock_try_acquire_recursive(_LOCK_T* lock) __attribute__ ((weak));
void __local_lock_release(_LOCK_T* lock) __attribute__ ((weak));
void __local_lock_release_recursive(_LOCK_T* lock) __attribute__ ((weak));

void __local_lock_init(_LOCK_T* lock) {
  return;
}

void __local_lock_init_recursive(_LOCK_T* lock) {
  return;
}

void __local_lock_close(_LOCK_T* lock) {
  return;
}

void __local_lock_close_recursive(_LOCK_T* lock) {
  return;
}

void __local_lock_acquire(_LOCK_T* lock) {
  return;
}

void __local_lock_acquire_recursive(_LOCK_T* lock) {
  return;
}

int __local_lock_try_acquire(_LOCK_T* lock) {
  return 0;
}

int __local_lock_try_acquire_recursive(_LOCK_T* lock) {
  return 0;
}

void __local_lock_release(_LOCK_T* lock) {
  return;
}

void __local_lock_release_recursive(_LOCK_T* lock) {
  return;
}
