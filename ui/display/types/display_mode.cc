// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/types/display_mode.h"

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"

namespace display {

DisplayMode::DisplayMode(const gfx::Size& size,
                         bool interlaced,
                         float refresh_rate)
    : size_(size), refresh_rate_(refresh_rate), is_interlaced_(interlaced) {}

DisplayMode::~DisplayMode() {}

std::unique_ptr<DisplayMode> DisplayMode::Clone() const {
  return base::WrapUnique(
      new DisplayMode(size_, is_interlaced_, refresh_rate_));
}

std::string DisplayMode::ToString() const {
  return base::StringPrintf("[%s %srate=%f]", size_.ToString().c_str(),
                            is_interlaced_ ? "interlaced " : "", refresh_rate_);
}

void PrintTo(const DisplayMode& mode, std::ostream* os) {
  *os << mode.ToString();
}

}  // namespace display
