// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_WATCHER_STABILITY_DEBUGGING_H_
#define COMPONENTS_BROWSER_WATCHER_STABILITY_DEBUGGING_H_

#include <stdint.h>

#include "base/strings/string_piece.h"

namespace browser_watcher {

// Adds or updates the global stability user data.
void SetStabilityDataBool(base::StringPiece name, bool value);
void SetStabilityDataInt(base::StringPiece name, int64_t value);

// Registers a vectored exception handler that stores exception details to the
// stability file.
void RegisterStabilityVEH();

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_STABILITY_DEBUGGING_H_
