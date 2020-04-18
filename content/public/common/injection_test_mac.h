// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_INJECTION_TEST_MAC_H_
#define CONTENT_PUBLIC_COMMON_INJECTION_TEST_MAC_H_

#import <Foundation/Foundation.h>

#include <string>

// Callback function used by sandbox tests to print error messages. It's
// provided in order to avoid linking the logging code of the renderer into this
// loadable bundle. RendererMainPlatformDelegate provides an implementation.
//
//   |message|  - the message that's printed.
//   |is_error| - true if this is an error message, false if an info message.
typedef void (*LogRendererSandboxTestMessage)(std::string message,
                                              bool is_error);

// An ObjC wrapper around sandbox tests.
@interface RendererSandboxTestsRunner : NSObject

// Sets the function that logs the progress of the tests.
+ (void)setLogFunction:(LogRendererSandboxTestMessage)logFunction;

// Runs all tests and logs its progress using the provided log function.
// Returns YES if all tests passed, NO otherwise. This method should be called
// after the sandbox has been turned on.
+ (BOOL)runTests;

@end

#endif  // CONTENT_PUBLIC_COMMON_INJECTION_TEST_MAC_H_
