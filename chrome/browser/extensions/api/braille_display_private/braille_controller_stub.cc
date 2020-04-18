// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/braille_display_private/braille_controller.h"

#include <memory>

#include "base/memory/singleton.h"
#include "chrome/browser/extensions/api/braille_display_private/stub_braille_controller.h"
#include "chrome/common/extensions/api/braille_display_private.h"

namespace extensions {
namespace api {
namespace braille_display_private {

BrailleController::BrailleController() {
}

BrailleController::~BrailleController() {
}

// static
BrailleController* BrailleController::GetInstance() {
  return StubBrailleController::GetInstance();
}

}  // namespace braille_display_private
}  // namespace api
}  // namespace extensions
