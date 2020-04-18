// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/synthetic_pen_driver.h"

namespace content {

SyntheticPenDriver::SyntheticPenDriver() : SyntheticMouseDriver() {
  mouse_event_.pointer_type = blink::WebPointerProperties::PointerType::kPen;
}

SyntheticPenDriver::~SyntheticPenDriver() {}

}  // namespace content
