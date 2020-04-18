// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/geometry/int_size.h"

#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d.h"

namespace blink {

IntSize::IntSize(const gfx::Size& size)
    : IntSize(size.width(), size.height()) {}

IntSize::operator gfx::Size() const {
  return gfx::Size(Width(), Height());
}

IntSize::operator gfx::Vector2d() const {
  return gfx::Vector2d(Width(), Height());
}

std::ostream& operator<<(std::ostream& ostream, const IntSize& size) {
  return ostream << size.ToString();
}

String IntSize::ToString() const {
  return String::Format("%dx%d", Width(), Height());
}

}  // namespace blink
