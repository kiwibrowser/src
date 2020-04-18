// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_TEST_RUNNER_EXPORT_H_
#define CONTENT_SHELL_TEST_RUNNER_TEST_RUNNER_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(TEST_RUNNER_IMPLEMENTATION)
#define TEST_RUNNER_EXPORT __declspec(dllexport)
#else
#define TEST_RUNNER_EXPORT __declspec(dllimport)
#endif  // defined(TEST_RUNNER_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(TEST_RUNNER_IMPLEMENTATION)
#define TEST_RUNNER_EXPORT __attribute__((visibility("default")))
#else
#define TEST_RUNNER_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define TEST_RUNNER_EXPORT
#endif

#endif  // CONTENT_SHELL_TEST_RUNNER_TEST_RUNNER_EXPORT_H_
