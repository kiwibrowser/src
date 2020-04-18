// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/window_activity_tracker_aura.h"

#include "base/logging.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/events/event_utils.h"

namespace content {

// static
std::unique_ptr<WindowActivityTracker> WindowActivityTracker::Create(
    gfx::NativeView window) {
  return std::unique_ptr<WindowActivityTracker>(
      new WindowActivityTrackerAura(window));
}

WindowActivityTrackerAura::WindowActivityTrackerAura(aura::Window* window)
    : window_(window),
      weak_factory_(this) {
  if (window_) {
    window_->AddObserver(this);
    window_->AddPreTargetHandler(this);
  }
}

WindowActivityTrackerAura::~WindowActivityTrackerAura() {
  if (window_) {
    window_->RemoveObserver(this);
    window_->RemovePreTargetHandler(this);
  }
}

base::WeakPtr<WindowActivityTracker> WindowActivityTrackerAura::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void WindowActivityTrackerAura::OnEvent(ui::Event* event) {
  switch (event->type()) {
    case ui::ET_MOUSE_PRESSED:
    case ui::ET_MOUSE_RELEASED:
    case ui::ET_MOUSE_MOVED:
      WindowActivityTracker::OnMouseActivity();
      break;
    default:
      break;
  }
}

void WindowActivityTrackerAura::OnWindowDestroying(aura::Window* window) {
  DCHECK_EQ(window_, window);
  window_->RemovePreTargetHandler(this);
  window_->RemoveObserver(this);
  window_ = nullptr;
}

}  // namespace content
