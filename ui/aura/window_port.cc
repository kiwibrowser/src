// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_port.h"

#include "ui/aura/window.h"

namespace aura {

// static
WindowPort* WindowPort::Get(Window* window) {
  return window ? window->port_ : nullptr;
}

// static
base::ObserverList<WindowObserver, true>* WindowPort::GetObservers(
    Window* window) {
  return &(window->observers_);
}

}  // namespace aura
