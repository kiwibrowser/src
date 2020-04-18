/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define NOMINMAX
// WinSock2.h must be included *BEFORE* windows.h
#include <winsock2.h>
#endif

#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.h>

#ifdef _WIN32
#pragma warning(push)
/*
    warnings 4251 and 4275 have to do with potential dll-interface mismatch
    between library (gtest) and users. Since we build the gtest library
    as part of the test build we know that the dll-interface will match and
    can disable these warnings.
 */
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "gtest/gtest.h"
#ifdef _WIN32
#pragma warning(pop)
#endif
#include "vktestbinding.h"

#define ASSERT_VK_SUCCESS(err) ASSERT_EQ(VK_SUCCESS, err) << vk_result_string(err)

static inline const char *vk_result_string(VkResult err) {
    switch (err) {
#define STR(r)                                                                                                                     \
    case r:                                                                                                                        \
        return #r
        STR(VK_SUCCESS);
        STR(VK_NOT_READY);
        STR(VK_TIMEOUT);
        STR(VK_EVENT_SET);
        STR(VK_EVENT_RESET);
        STR(VK_ERROR_INITIALIZATION_FAILED);
        STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        STR(VK_ERROR_DEVICE_LOST);
        STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        STR(VK_ERROR_LAYER_NOT_PRESENT);
        STR(VK_ERROR_MEMORY_MAP_FAILED);
        STR(VK_ERROR_INCOMPATIBLE_DRIVER);
#undef STR
    default:
        return "UNKNOWN_RESULT";
    }
}

static inline void test_error_callback(const char *expr, const char *file, unsigned int line, const char *function) {
    ADD_FAILURE_AT(file, line) << "Assertion: `" << expr << "'";
}

#if defined(__linux__)
/* Linux-specific common code: */

#include <pthread.h>

// Threads:
typedef pthread_t test_platform_thread;

static inline int test_platform_thread_create(test_platform_thread *thread, void *(*func)(void *), void *data) {
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    return pthread_create(thread, &thread_attr, func, data);
}
static inline int test_platform_thread_join(test_platform_thread thread, void **retval) { return pthread_join(thread, retval); }

// Thread IDs:
typedef pthread_t test_platform_thread_id;
static inline test_platform_thread_id test_platform_get_thread_id() { return pthread_self(); }

// Thread mutex:
typedef pthread_mutex_t test_platform_thread_mutex;
static inline void test_platform_thread_create_mutex(test_platform_thread_mutex *pMutex) { pthread_mutex_init(pMutex, NULL); }
static inline void test_platform_thread_lock_mutex(test_platform_thread_mutex *pMutex) { pthread_mutex_lock(pMutex); }
static inline void test_platform_thread_unlock_mutex(test_platform_thread_mutex *pMutex) { pthread_mutex_unlock(pMutex); }
static inline void test_platform_thread_delete_mutex(test_platform_thread_mutex *pMutex) { pthread_mutex_destroy(pMutex); }
typedef pthread_cond_t test_platform_thread_cond;
static inline void test_platform_thread_init_cond(test_platform_thread_cond *pCond) { pthread_cond_init(pCond, NULL); }
static inline void test_platform_thread_cond_wait(test_platform_thread_cond *pCond, test_platform_thread_mutex *pMutex) {
    pthread_cond_wait(pCond, pMutex);
}
static inline void test_platform_thread_cond_broadcast(test_platform_thread_cond *pCond) { pthread_cond_broadcast(pCond); }

#elif defined(_WIN32) // defined(__linux__)
// Threads:
typedef HANDLE test_platform_thread;
static inline int test_platform_thread_create(test_platform_thread *thread, void *(*func)(void *), void *data) {
    DWORD threadID;
    *thread = CreateThread(NULL, // default security attributes
                           0,    // use default stack size
                           (LPTHREAD_START_ROUTINE)func,
                           data,       // thread function argument
                           0,          // use default creation flags
                           &threadID); // returns thread identifier
    return (*thread != NULL);
}
static inline int test_platform_thread_join(test_platform_thread thread, void **retval) {
    return WaitForSingleObject(thread, INFINITE);
}

// Thread IDs:
typedef DWORD test_platform_thread_id;
static test_platform_thread_id test_platform_get_thread_id() { return GetCurrentThreadId(); }

// Thread mutex:
typedef CRITICAL_SECTION test_platform_thread_mutex;
static void test_platform_thread_create_mutex(test_platform_thread_mutex *pMutex) { InitializeCriticalSection(pMutex); }
static void test_platform_thread_lock_mutex(test_platform_thread_mutex *pMutex) { EnterCriticalSection(pMutex); }
static void test_platform_thread_unlock_mutex(test_platform_thread_mutex *pMutex) { LeaveCriticalSection(pMutex); }
static void test_platform_thread_delete_mutex(test_platform_thread_mutex *pMutex) { DeleteCriticalSection(pMutex); }
typedef CONDITION_VARIABLE test_platform_thread_cond;
static void test_platform_thread_init_cond(test_platform_thread_cond *pCond) { InitializeConditionVariable(pCond); }
static void test_platform_thread_cond_wait(test_platform_thread_cond *pCond, test_platform_thread_mutex *pMutex) {
    SleepConditionVariableCS(pCond, pMutex, INFINITE);
}
static void test_platform_thread_cond_broadcast(test_platform_thread_cond *pCond) { WakeAllConditionVariable(pCond); }
#else                 // defined(_WIN32)

#error The "test_common.h" file must be modified for this OS.

// NOTE: In order to support another OS, an #elif needs to be added (above the
// "#else // defined(_WIN32)") for that OS, and OS-specific versions of the
// contents of this file must be created.

// NOTE: Other OS-specific changes are also needed for this OS.  Search for
// files with "WIN32" in it, as a quick way to find files that must be changed.

#endif // defined(_WIN32)

#endif // TEST_COMMON_H
