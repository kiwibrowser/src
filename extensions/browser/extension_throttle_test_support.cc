// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_throttle_test_support.h"

namespace extensions {

TestTickClock::TestTickClock() {
}

TestTickClock::TestTickClock(base::TimeTicks now) : now_ticks_(now) {
}

TestTickClock::~TestTickClock() {
}

base::TimeTicks TestTickClock::NowTicks() const {
  return now_ticks_;
}

}  // namespace extensions
