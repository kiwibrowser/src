// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace/workspace_event_handler_mash.h"

#include "ui/aura/window.h"
#include "ui/base/class_property.h"

DEFINE_UI_CLASS_PROPERTY_TYPE(ash::WorkspaceEventHandlerMash*);

namespace {

DEFINE_UI_CLASS_PROPERTY_KEY(ash::WorkspaceEventHandlerMash*,
                             kWorkspaceEventHandlerProperty,
                             nullptr);

}  // namespace

namespace ash {

WorkspaceEventHandlerMash::WorkspaceEventHandlerMash(
    aura::Window* workspace_window)
    : workspace_window_(workspace_window) {
  workspace_window_->SetProperty(kWorkspaceEventHandlerProperty, this);
}

WorkspaceEventHandlerMash::~WorkspaceEventHandlerMash() {
  workspace_window_->ClearProperty(kWorkspaceEventHandlerProperty);
}

// static
WorkspaceEventHandlerMash* WorkspaceEventHandlerMash::Get(
    aura::Window* window) {
  return window->GetProperty(kWorkspaceEventHandlerProperty);
}

}  // namespace ash
