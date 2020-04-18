// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/view_tracker.h"

#include "ui/views/view.h"

namespace views {

ViewTracker::ViewTracker(View* view) : view_(nullptr) {
  SetView(view);
}

ViewTracker::~ViewTracker() {
  SetView(nullptr);
}

void ViewTracker::SetView(View* view) {
  if (view == view_)
    return;

  if (view_)
    view_->RemoveObserver(this);
  view_ = view;
  if (view_)
    view_->AddObserver(this);
}

void ViewTracker::OnViewIsDeleting(View* observed_view) {
  SetView(nullptr);
}

}  // namespace views
