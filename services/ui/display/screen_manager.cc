// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/display/screen_manager.h"

#include "base/logging.h"

namespace display {

// static
ScreenManager* ScreenManager::instance_ = nullptr;

ScreenManager::ScreenManager() {
  DCHECK(!instance_);
  instance_ = this;
}

ScreenManager::~ScreenManager() {
  DCHECK_EQ(instance_, this);
  instance_ = nullptr;
}

// static
ScreenManager* ScreenManager::GetInstance() {
  DCHECK(instance_);
  return instance_;
}

}  // namespace display
