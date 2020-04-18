// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_GOOGLETEST_CUSTOM_GTEST_INTERNAL_CUSTOM_GTEST_H_
#define THIRD_PARTY_GOOGLETEST_CUSTOM_GTEST_INTERNAL_CUSTOM_GTEST_H_

#include "build/build_config.h"

#if !defined(GTEST_DISABLE_PRINT_STACK_TRACE)
#include "third_party/googletest/custom/gtest/internal/custom/stack_trace_getter.h"

// Tell Google Test to use a stack trace getter based on Chromium's
// base::debug::StackTrace.
#define GTEST_OS_STACK_TRACE_GETTER_ StackTraceGetter
#endif  // defined(GTEST_DISABLE_PRINT_STACK_TRACE)

#endif  // THIRD_PARTY_GOOGLETEST_CUSTOM_GTEST_INTERNAL_CUSTOM_GTEST_H_
