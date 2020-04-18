// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/scrollbar_manager.h"

namespace blink {

ScrollbarManager::ScrollbarManager(ScrollableArea& scrollable_area)
    : scrollable_area_(&scrollable_area),
      h_bar_is_attached_(0),
      v_bar_is_attached_(0) {}

void ScrollbarManager::Trace(blink::Visitor* visitor) {
  visitor->Trace(scrollable_area_);
  visitor->Trace(h_bar_);
  visitor->Trace(v_bar_);
}

void ScrollbarManager::Dispose() {
  h_bar_is_attached_ = v_bar_is_attached_ = 0;
  DestroyScrollbar(kHorizontalScrollbar);
  DestroyScrollbar(kVerticalScrollbar);
}

}  // namespace blink
