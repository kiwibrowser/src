// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/window_manager_frame_values.h"

#include "base/lazy_instance.h"

namespace views {
namespace {

base::LazyInstance<WindowManagerFrameValues>::Leaky lazy_instance =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

WindowManagerFrameValues::WindowManagerFrameValues()
    : max_title_bar_button_width(0) {}

WindowManagerFrameValues::~WindowManagerFrameValues() {}

// static
void WindowManagerFrameValues::SetInstance(
    const WindowManagerFrameValues& values) {
  lazy_instance.Get() = values;
}

// static
const WindowManagerFrameValues& WindowManagerFrameValues::instance() {
  return lazy_instance.Get();
}

}  // namespace views
