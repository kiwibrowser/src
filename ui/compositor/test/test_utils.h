// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_TEST_TEST_UTILS_H_
#define UI_COMPOSITOR_TEST_TEST_UTILS_H_

namespace gfx {
class Rect;
class Transform;
}

namespace ui {

void CheckApproximatelyEqual(const gfx::Transform& lhs,
                             const gfx::Transform& rhs);
void CheckApproximatelyEqual(const gfx::Rect& lhs, const gfx::Rect& rhs);

}  // namespace ui

#endif  // UI_COMPOSITOR_TEST_TEST_UTILS_H_
