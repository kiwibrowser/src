/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Thread support for gl dispatch.
 *
 * Initial version by John Stone (j.stone@acm.org) (johns@cs.umr.edu)
 *                and Christoph Poliwoda (poliwoda@volumegraphics.com)
 * Revised by Keith Whitwell
 * Adapted for new gl dispatcher by Brian Paul
 * Modified for use in mapi by Chia-I Wu
 */

/*
 * If this file is accidentally included by a non-threaded build,
 * it should not cause the build to fail, or otherwise cause problems.
 * In general, it should only be included when needed however.
 */

#ifndef _U_THREAD_H_
#define _U_THREAD_H_

#include <stdio.h>
#include <stdlib.h>
#include "u_compiler.h"

#if defined(HAVE_PTHREAD)
#include <pthread.h> /* POSIX threads headers */
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#if defined(HAVE_PTHREAD) || defined(_WIN32)
#ifndef THREADS
#define THREADS
#endif
#endif

/*
 * Error messages
 */
#define INIT_TSD_ERROR "_glthread_: failed to allocate key for thread specific data"
#define GET_TSD_ERROR "_glthread_: failed to get thread specific data"
#define SET_TSD_ERROR "_glthread_: thread failed to set thread specific data"


/*
 * Magic number to determine if a TSD object has been initialized.
 * Kind of a hack but there doesn't appear to be a better cross-platform
 * solution.
 */
#define INIT_MAGIC 0xff8adc98

#ifdef __cplusplus
extern "C" {
#endif


/*
 * POSIX threads. This should be your choice in the Unix world
 * whenever possible.  When building with POSIX threads, be sure
 * to enable any compiler flags which will cause the MT-safe
 * libc (if one exists) to be used when linking, as well as any
 * header macros for MT-safe errno, etc.  For Solaris, this is the -mt
 * compiler flag.  On Solaris with gcc, use -D_REENTRANT to enable
 * proper compiling for MT-safe libc etc.
 */
#if defined(HAVE_PTHREAD)

struct u_tsd {
   pthread_key_t key;
   unsigned initMagic;
};

typedef pthread_mutex_t u_mutex;

#define u_mutex_declare_static(name) \
   static u_mutex name = PTHREAD_MUTEX_INITIALIZER

#define u_mutex_init(name)    pthread_mutex_init(&(name), NULL)
#define u_mutex_destroy(name) pthread_mutex_destroy(&(name))
#define u_mutex_lock(name)    (void) pthread_mutex_lock(&(name))
#define u_mutex_unlock(name)  (void) pthread_mutex_unlock(&(name))

static INLINE unsigned long
u_thread_self(void)
{
   return (unsigned long) pthread_self();
}


static INLINE void
u_tsd_init(struct u_tsd *tsd)
{
   if (pthread_key_create(&tsd->key, NULL/*free*/) != 0) {
      perror(INIT_TSD_ERROR);
      exit(-1);
   }
   tsd->initMagic = INIT_MAGIC;
}


static INLINE void *
u_tsd_get(struct u_tsd *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   return pthread_getspecific(tsd->key);
}


static INLINE void
u_tsd_set(struct u_tsd *tsd, void *ptr)
{
   if (tsd->initMagic != INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   if (pthread_setspecific(tsd->key, ptr) != 0) {
      perror(SET_TSD_ERROR);
      exit(-1);
   }
}

#endif /* HAVE_PTHREAD */


/*
 * Windows threads. Should work with Windows NT and 95.
 * IMPORTANT: Link with multithreaded runtime library when THREADS are
 * used!
 */
#ifdef WIN32

struct u_tsd {
   DWORD key;
   unsigned initMagic;
};

typedef CRITICAL_SECTION u_mutex;

/* http://locklessinc.com/articles/pthreads_on_windows/ */
#define u_mutex_declare_static(name) \
   static u_mutex name = {(PCRITICAL_SECTION_DEBUG)-1, -1, 0, 0, 0, 0}

#define u_mutex_init(name)    InitializeCriticalSection(&name)
#define u_mutex_destroy(name) DeleteCriticalSection(&name)
#define u_mutex_lock(name)    EnterCriticalSection(&name)
#define u_mutex_unlock(name)  LeaveCriticalSection(&name)

static INLINE unsigned long
u_thread_self(void)
{
   return GetCurrentThreadId();
}


static INLINE void
u_tsd_init(struct u_tsd *tsd)
{
   tsd->key = TlsAlloc();
   if (tsd->key == TLS_OUT_OF_INDEXES) {
      perror(INIT_TSD_ERROR);
      exit(-1);
   }
   tsd->initMagic = INIT_MAGIC;
}


static INLINE void
u_tsd_destroy(struct u_tsd *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      return;
   }
   TlsFree(tsd->key);
   tsd->initMagic = 0x0;
}


static INLINE void *
u_tsd_get(struct u_tsd *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   return TlsGetValue(tsd->key);
}


static INLINE void
u_tsd_set(struct u_tsd *tsd, void *ptr)
{
   /* the following code assumes that the struct u_tsd has been initialized
      to zero at creation */
   if (tsd->initMagic != INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   if (TlsSetValue(tsd->key, ptr) == 0) {
      perror(SET_TSD_ERROR);
      exit(-1);
   }
}

#endif /* WIN32 */


/*
 * THREADS not defined
 */
#ifndef THREADS

struct u_tsd {
   unsigned initMagic;
};

typedef unsigned u_mutex;

#define u_mutex_declare_static(name)   static u_mutex name = 0
#define u_mutex_init(name)             (void) name
#define u_mutex_destroy(name)          (void) name
#define u_mutex_lock(name)             (void) name
#define u_mutex_unlock(name)           (void) name

/*
 * no-op functions
 */

static INLINE unsigned long
u_thread_self(void)
{
   return 0;
}


static INLINE void
u_tsd_init(struct u_tsd *tsd)
{
   (void) tsd;
}


static INLINE void *
u_tsd_get(struct u_tsd *tsd)
{
   (void) tsd;
   return NULL;
}


static INLINE void
u_tsd_set(struct u_tsd *tsd, void *ptr)
{
   (void) tsd;
   (void) ptr;
}
#endif /* THREADS */


#ifdef __cplusplus
}
#endif

#endif /* _U_THREAD_H_ */
