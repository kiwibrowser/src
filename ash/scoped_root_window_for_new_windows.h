// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SCOPED_ROOT_WINDOW_FOR_NEW_WINDOWS_H_
#define ASH_SCOPED_ROOT_WINDOW_FOR_NEW_WINDOWS_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace aura {
class Window;
}

namespace ash {

// Constructing a ScopedRootWindowForNewWindows allows temporarily
// switching a target root window so that a new window gets created
// in the same window where a user interaction happened.
// An example usage is to specify the target root window when creating
// a new window using launcher's icon.
class ASH_EXPORT ScopedRootWindowForNewWindows {
 public:
  explicit ScopedRootWindowForNewWindows(aura::Window* new_root);
  ~ScopedRootWindowForNewWindows();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedRootWindowForNewWindows);
};

}  // namespace ash

#endif  // ASH_SCOPED_ROOT_WINDOW_FOR_NEW_WINDOWS_H_
