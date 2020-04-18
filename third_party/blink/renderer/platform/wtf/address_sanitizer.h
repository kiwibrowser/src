// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ADDRESS_SANITIZER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ADDRESS_SANITIZER_H_
// TODO(kojii): This file will need to be renamed, because it's no more
// specific to AddressSanitizer.

// TODO(sof): Add SyZyASan support?
#if defined(ADDRESS_SANITIZER)
#include <sanitizer/asan_interface.h>
#define NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#define NO_SANITIZE_ADDRESS
#endif

#if defined(LEAK_SANITIZER)
#include <sanitizer/lsan_interface.h>
#else
#define __lsan_register_root_region(addr, size) ((void)(addr), (void)(size))
#define __lsan_unregister_root_region(addr, size) ((void)(addr), (void)(size))
#endif

#if defined(MEMORY_SANITIZER)
#include <sanitizer/msan_interface.h>
#define NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#else
#define NO_SANITIZE_MEMORY
#endif

#if defined(THREAD_SANITIZER)
#define NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define NO_SANITIZE_THREAD
#endif

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ADDRESS_SANITIZER_H_
