// Copyright (C) 2011 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
// One-time construction C++ runtime support
// See "3.3.2 One-time Construction API" of the Itanium C++ ABI reference
// And "3.2.3 Guard variables and the one-time construction API" in the ARM C++ ABI reference.

/* Note that the ARM C++ ABI defines the size of each guard variable
 * as 32-bit, while the generic/Itanium one defines it as 64-bit.
 *
 * Also the ARM C++ ABI uses the least-significant bit to indicate
 * completion, while the generic/Itanium one uses the least-significant
 * byte. In all cases the corresponding item is set to value '1'
 *
 * We will treat guard variables here as 32-bit values, even on x86,
 * given that this representation is compatible with compiler-generated
 * variables that are 64-bits on little-endian systems. This makes the
 * code simpler and slightly more efficient
 */

#include <stddef.h>
#include <pthread.h>

// Temporary hack since we build this against prebuilts/ndk when building the
// NDK but against the shipped platforms in the tests (FORCE_REBUILD case). We
// need to update the headers in prebuilts/ndk, but we can't do that until we
// have an NDK built with the unified headers.
#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#endif

/* In this implementation, we use a single global mutex+condvar pair.
 *
 * Pros: portable and doesn't require playing with futexes, atomics
 *       and memory barriers.
 *
 * Cons: Slower than necessary.
 */
static pthread_mutex_t  sMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static pthread_cond_t   sCond  = PTHREAD_COND_INITIALIZER;


extern "C" int __cxa_guard_acquire(int volatile * gv)
{
    pthread_mutex_lock(&sMutex);
    for (;;) {
        // while gv points to a volatile value, we use the
        // previous pthread_mutex_lock or pthread_cond_wait
        // as a trivial memory barrier
        int guard = *gv;
        if ((guard & 1) != 0) {
            /* already initialized - return 0 */
            pthread_mutex_unlock(&sMutex);
            return 0;
        }

        // we use bit 8 to indicate that the guard value is being
        // initialized, and bit 9 to indicate that there is another
        // thread waiting for its completion.
        if ((guard & 0x100) == 0) {
            // nobody is initializing this yet, so mark the guard value
            // first. and allow initialization to proceed.
            *gv = 0x100;
            pthread_mutex_unlock(&sMutex);
            return 1;
        }

        // already being initialized by amother thread,
        // we must indicate that there is a waiter, then
        // wait to be woken up before trying again.
        *gv = guard | 0x200;
        pthread_cond_wait(&sCond, &sMutex);
    }
}

extern "C" void __cxa_guard_release(int volatile * gv)
{
    pthread_mutex_lock(&sMutex);

    int guard = *gv;
    // this indicates initialization for our two ABIs.
    *gv = 0x1;
    if ((guard & 0x200) != 0)
        pthread_cond_broadcast(&sCond);

    pthread_mutex_unlock(&sMutex);
}

extern "C" void __cxa_guard_abort(int volatile * gv)
{
    pthread_mutex_lock(&sMutex);

    int guard = *gv;
    *gv = 0;
    if ((guard & 0x200) != 0)
        pthread_cond_broadcast(&sCond);

    pthread_mutex_unlock(&sMutex);
}
