// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WINDOW_PARENTING_UTILS_H_
#define ASH_WM_WINDOW_PARENTING_UTILS_H_

namespace aura {
class Window;
}

namespace ash {

namespace wm {

// Changes the parent of a |child| and all its transient children that are
// themselves children of |old_parent| to |new_parent|.
void ReparentChildWithTransientChildren(aura::Window* child,
                                        aura::Window* old_parent,
                                        aura::Window* new_parent);

// Changes the parent of all transient children of a |child| to |new_parent|.
// Does not change parent of the transient children that are not themselves
// children of |old_parent|.
void ReparentTransientChildrenOfChild(aura::Window* child,
                                      aura::Window* old_parent,
                                      aura::Window* new_parent);

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_WINDOW_PARENTING_UTILS_H_
