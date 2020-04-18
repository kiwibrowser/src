// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/test_omnibox_edit_controller.h"

TestToolbarModel* TestOmniboxEditController::GetToolbarModel() {
  return &toolbar_model_;
}

const TestToolbarModel* TestOmniboxEditController::GetToolbarModel() const {
  return &toolbar_model_;
}
