// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/content/shell_content_state.h"

#include "base/logging.h"
#include "ui/keyboard/keyboard_resource_util.h"

namespace ash {

// static
ShellContentState* ShellContentState::instance_ = nullptr;

// static
void ShellContentState::SetInstance(ShellContentState* instance) {
  DCHECK(!instance_);
  instance_ = instance;
}

// static
ShellContentState* ShellContentState::GetInstance() {
  DCHECK(instance_);
  return instance_;
}

// static
void ShellContentState::DestroyInstance() {
  DCHECK(instance_);
  delete instance_;
  instance_ = nullptr;
}

ShellContentState::ShellContentState() {
  // The keyboard system must be initialized before the RootWindowController is
  // created.
  keyboard::InitializeKeyboardResources();
}

ShellContentState::~ShellContentState() = default;

}  // namespace ash
