// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGCaretRect_h
#define NGCaretRect_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/editing/forward.h"

namespace blink {

// This file provides utility functions for computing caret rect in LayoutNG.

struct LocalCaretRect;

// Given a position with affinity, returns the caret rect if the position is
// laid out with LayoutNG, and a caret can be placed at the position with the
// given affinity. The caret rect location is local to the containing inline
// formatting context.
CORE_EXPORT LocalCaretRect ComputeNGLocalCaretRect(const PositionWithAffinity&);

}  // namespace blink

#endif  // NGCaretRect_h
