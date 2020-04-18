// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_API_SELECTION_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_API_SELECTION_STATE_H_

#include <iosfwd>
#include "third_party/blink/renderer/core/core_export.h"

namespace blink {

enum class SelectionState {
  // LayoutObject is not selected.
  kNone,
  // LayoutObject is the start of a selection run and doesn't have children.
  kStart,
  // LayoutObject is fully encompassed by a selection run and
  // doesn't have children.
  kInside,
  // LayoutObject is the end of a selection run and doesn't have children.
  kEnd,
  // LayoutObject contains an entire selection run and doesn't have children.
  kStartAndEnd,
  // LayoutObject has at least one LayoutObject child which SelectionState is
  // kStart, kInside, kEnd or kStartAndEnd.
  // This property is used to invalidate LayoutObject for ::selection style
  // change. See LayoutObject::InvalidatePaintForSelection().
  kContain
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const SelectionState);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_API_SELECTION_STATE_H_
