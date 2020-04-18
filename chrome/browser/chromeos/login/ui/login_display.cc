// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display.h"

namespace chromeos {

LoginDisplay::Delegate::~Delegate() = default;

LoginDisplay::LoginDisplay(Delegate* delegate) : delegate_(delegate) {}

LoginDisplay::~LoginDisplay() = default;

}  // namespace chromeos
