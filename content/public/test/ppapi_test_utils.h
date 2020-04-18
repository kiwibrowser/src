// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_PPAPI_TEST_UTILS_H_
#define CONTENT_PUBLIC_TEST_PPAPI_TEST_UTILS_H_

#include "base/compiler_specific.h"
#include "base/files/file_path.h"

namespace base {
class CommandLine;
}

// This file specifies utility functions used in Pepper testing in
// browser_tests and content_browsertests.
namespace ppapi {

// Registers the PPAPI test plugin to application/x-ppapi-tests. Returns true
// on success, and false otherwise.
bool RegisterTestPlugin(base::CommandLine* command_line) WARN_UNUSED_RESULT;

// Registers the PPAPI test plugin with some some extra parameters. Returns true
// on success and false otherwise.
bool RegisterTestPluginWithExtraParameters(
    base::CommandLine* command_line,
    const base::FilePath::StringType& extra_registration_parameters)
    WARN_UNUSED_RESULT;

// Registers the Flash-imitating Power Saver test plugin.
bool RegisterFlashTestPlugin(base::CommandLine* command_line)
    WARN_UNUSED_RESULT;

// Registers the Blink test plugin to application/x-blink-test-plugin.
bool RegisterBlinkTestPlugin(base::CommandLine* command_line)
    WARN_UNUSED_RESULT;

}  // namespace ppapi

#endif  // CONTENT_PUBLIC_TEST_PPAPI_TEST_UTILS_H_
