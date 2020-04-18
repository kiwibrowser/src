// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_FOCUS_RULES_H_
#define ASH_WM_FOCUS_RULES_H_

#include "ash/ash_export.h"

namespace aura {
class Window;
}

namespace ash {

// These functions provide the ash implementation wm::FocusRules. See
// description there for details.
ASH_EXPORT bool IsToplevelWindow(aura::Window* window);
ASH_EXPORT bool IsWindowConsideredActivatable(aura::Window* window);
ASH_EXPORT bool IsWindowConsideredVisibleForActivation(aura::Window* window);

}  // namespace ash

#endif  // ASH_WM_FOCUS_RULES_H_
