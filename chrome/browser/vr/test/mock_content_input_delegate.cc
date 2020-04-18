// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/test/mock_content_input_delegate.h"

#include "third_party/blink/public/platform/web_gesture_event.h"
#include "ui/gfx/geometry/point_f.h"

namespace vr {

MockContentInputDelegate::MockContentInputDelegate()
    : ContentInputDelegate(nullptr) {}
MockContentInputDelegate::~MockContentInputDelegate() {}

}  // namespace vr
