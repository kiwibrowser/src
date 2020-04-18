// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_UTIL_H_
#define CHROMEOS_BINDER_UTIL_H_

#include <stdint.h>

#include "chromeos/chromeos_export.h"

namespace binder {

// Returns the string representation of the given binder command or "UNKNOWN"
// if command is unknown, never returns null.
CHROMEOS_EXPORT const char* CommandToString(uint32_t command);

}  // namespace binder

#endif  // CHROMEOS_BINDER_UTIL_H_
