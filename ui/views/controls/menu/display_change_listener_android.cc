// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/menu/menu_runner.h"

namespace views {
namespace internal {

// static
DisplayChangeListener* DisplayChangeListener::Create(Widget*, MenuRunner*) {
  return NULL;
}

}  // namespace internal
}  // namespace views
