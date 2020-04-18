// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/closure_animation_observer.h"

namespace ui {

ClosureAnimationObserver::ClosureAnimationObserver(const base::Closure& closure)
    : closure_(closure) {
  DCHECK(!closure_.is_null());
}

ClosureAnimationObserver::~ClosureAnimationObserver() {
}

void ClosureAnimationObserver::OnImplicitAnimationsCompleted() {
  closure_.Run();
  delete this;
}

}  // namespace ui
