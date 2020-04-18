// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/fullscreen.h"

#include "ui/aura/env.h"

bool IsFullScreenMode() {
  if (aura::Env::GetInstance()->mode() == aura::Env::Mode::MUS) {
    // TODO: http://crbug.com/640390.
    NOTIMPLEMENTED();
    return false;
  }

  // TODO: https://crbug.com/843018
  NOTIMPLEMENTED();
  return false;
}
