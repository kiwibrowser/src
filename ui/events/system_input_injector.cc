// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/system_input_injector.h"


namespace ui {

namespace {
SystemInputInjectorFactory* override_factory_ = nullptr;
}  // namespace

void SetSystemInputInjectorFactory(SystemInputInjectorFactory* factory) {
  DCHECK(!factory || !override_factory_);
  override_factory_ = factory;
}

SystemInputInjectorFactory* GetSystemInputInjectorFactory() {
  return override_factory_;
}

}  // namespace ui
