// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_LAYOUT_UTILS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_LAYOUT_UTILS_H_

namespace blink {

class NGConstraintSpace;
class NGLayoutResult;

// Return true if layout is considered complete. In some cases we require more
// than one layout pass.
// This function never considers intermediate layouts
// (space,IsIntermediateLayout()) to be complete.
bool IsBlockLayoutComplete(const NGConstraintSpace&, const NGLayoutResult&);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_LAYOUT_UTILS_H_
