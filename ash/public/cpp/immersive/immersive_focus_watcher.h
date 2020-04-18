// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_IMMERSIVE_IMMERSIVE_FOCUS_WATCHER_H_
#define ASH_PUBLIC_CPP_IMMERSIVE_IMMERSIVE_FOCUS_WATCHER_H_

#include "ash/public/cpp/ash_public_export.h"

namespace ash {

// ImmersiveFocusWatcher is responsible for grabbing a reveal lock based on
// activation and/or focus.
class ASH_PUBLIC_EXPORT ImmersiveFocusWatcher {
 public:
  virtual ~ImmersiveFocusWatcher() {}

  // Forces updating the status of the lock. That is, this determines whether
  // a lock should be held and updates accordingly. The lock is automatically
  // maintained, but this function may be called to force an update.
  virtual void UpdateFocusRevealedLock() = 0;

  // Explicitly releases the lock, does nothing if a lock is not held.
  virtual void ReleaseLock() = 0;
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_IMMERSIVE_IMMERSIVE_FOCUS_WATCHER_H_
