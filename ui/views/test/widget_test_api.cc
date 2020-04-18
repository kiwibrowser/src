// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/widget.h"

namespace views {

void DisableActivationChangeHandlingForTests() {
  Widget::g_disable_activation_change_handling_ = true;
}

}  // namespace views
