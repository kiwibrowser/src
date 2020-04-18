// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_observer.h"

#include "base/logging.h"
#include "ui/aura/window.h"

namespace aura {

WindowObserver::WindowObserver() : observing_(0) {
}

WindowObserver::~WindowObserver() {
  CHECK_EQ(0, observing_);
}

void WindowObserver::OnObservingWindow(aura::Window* window) {
  if (!window->HasObserver(this))
    observing_++;
}

void WindowObserver::OnUnobservingWindow(aura::Window* window) {
  if (window->HasObserver(this))
    observing_--;
}

void WindowObserver::OnEmbeddedAppDisconnected(Window* window) {}

}  // namespace aura
