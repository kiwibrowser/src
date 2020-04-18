// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/layout_manager.h"

#include "ui/aura/window.h"

namespace aura {

LayoutManager::LayoutManager() {
}

LayoutManager::~LayoutManager() {
}

void LayoutManager::SetChildBoundsDirect(aura::Window* child,
                                         const gfx::Rect& bounds) {
  child->SetBoundsInternal(bounds);
}

}  // namespace aura
