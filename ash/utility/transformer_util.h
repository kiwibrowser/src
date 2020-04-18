// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TRANSFORMER_UTIL_H_
#define ASH_TRANSFORMER_UTIL_H_

#include "ash/ash_export.h"
#include "ui/display/display.h"

namespace gfx {
class Transform;
}

namespace ash {

// Creates rotation transform that rotates the |rect_to_rotate| from
// |old_rotation| to |new_rotation|.
ASH_EXPORT gfx::Transform CreateRotationTransform(
    display::Display::Rotation old_rotation,
    display::Display::Rotation new_rotation,
    const gfx::Rect& rect_to_rotate);

}  // namespace ash

#endif  // ASH_TRANSFORMER_UTIL_H_
