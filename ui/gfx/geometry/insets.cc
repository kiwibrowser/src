// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/geometry/insets.h"

#include "base/strings/stringprintf.h"
#include "ui/gfx/geometry/vector2d.h"

namespace gfx {

std::string Insets::ToString() const {
  // Print members in the same order of the constructor parameters.
  return base::StringPrintf("%d,%d,%d,%d", top(),  left(), bottom(), right());
}

Insets Insets::Offset(const gfx::Vector2d& vector) const {
  return gfx::Insets(top() + vector.y(), left() + vector.x(),
                     bottom() - vector.y(), right() - vector.x());
}

}  // namespace gfx
