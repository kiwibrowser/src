// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_parenting_utils.h"

#include "ui/aura/window.h"
#include "ui/wm/core/window_util.h"

using aura::Window;

namespace ash {
namespace wm {

void ReparentChildWithTransientChildren(Window* child,
                                        Window* old_parent,
                                        Window* new_parent) {
  if (child->parent() == old_parent)
    new_parent->AddChild(child);
  ReparentTransientChildrenOfChild(child, old_parent, new_parent);
}

void ReparentTransientChildrenOfChild(Window* child,
                                      Window* old_parent,
                                      Window* new_parent) {
  for (Window* transient_child : ::wm::GetTransientChildren(child))
    ReparentChildWithTransientChildren(transient_child, old_parent, new_parent);
}

}  // namespace wm
}  // namespace ash
