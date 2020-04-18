// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_UTIL_H_
#define COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_UTIL_H_

#include "jingle/notifier/base/notifier_options.h"

namespace base {
class CommandLine;
}

namespace invalidation {

// Parses the given command line for notifier options.
notifier::NotifierOptions ParseNotifierOptions(
    const base::CommandLine& command_line);

// Generates a unique client ID for the invalidator.
std::string GenerateInvalidatorClientId();

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_UTIL_H_
