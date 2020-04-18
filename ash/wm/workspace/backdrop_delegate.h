// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WORKSPACE_WORKSPACE_LAYOUT_MANAGER_BACKDROP_DELEGATE_H_
#define ASH_WM_WORKSPACE_WORKSPACE_LAYOUT_MANAGER_BACKDROP_DELEGATE_H_

#include "ash/ash_export.h"

namespace aura {
class Window;
}

namespace ash {

// A delegate which can be set to create and control a backdrop which gets
// placed below the top level window.
class ASH_EXPORT BackdropDelegate {
 public:
  virtual ~BackdropDelegate() {}

  // Returns true if |window| should have a backdrop.
  virtual bool HasBackdrop(aura::Window* window) = 0;
};

}  // namespace ash

#endif  // ASH_WM_WORKSPACE_WORKSPACE_LAYOUT_MANAGER_BACKDROP_DELEGATE_H_
