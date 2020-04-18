// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CRASH_HELPERS_H_
#define CONTENT_RENDERER_CRASH_HELPERS_H_

#include "base/compiler_specific.h"

namespace content {

namespace internal {

// These functions are defined in a separate file from the calling
// file to try to prevent the compiler from inlining them. Some tests
// which verify crashpad, and stack symbolization from minidumps,
// expect to find the names of these functions.

NOINLINE void CrashIntentionally();
NOINLINE void BadCastCrashIntentionally();

}  // namespace internal

}  // namespace content

#endif  // CONTENT_RENDERER_CRASH_HELPERS_H_
