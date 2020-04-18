// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/sidebar/sidebar.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/sidebar/sidebar_widget.h"
#include "ui/aura/window.h"

namespace ash {

Sidebar::Sidebar() {}
Sidebar::~Sidebar() {}

void Sidebar::SetShelf(Shelf* shelf) {
  DCHECK(!widget_);
  DCHECK(!shelf_);

  shelf_ = shelf;
}

void Sidebar::Show(SidebarInitMode mode) {
  CHECK(switches::IsSidebarEnabled());
  DCHECK(shelf_);

  if (!widget_) {
    aura::Window* container =
        shelf_->GetWindow()->GetRootWindow()->GetChildById(
            kShellWindowId_ShelfBubbleContainer);
    widget_ = new SidebarWidget(container, this, shelf_, mode);
    widget_->Show();
  } else {
    widget_->Reinitialize(mode);
  }
}

void Sidebar::Hide() {
  CHECK(switches::IsSidebarEnabled());
  DCHECK(widget_);

  widget_->Close();
  widget_ = nullptr;
}

bool Sidebar::IsVisible() const {
  return widget_ && widget_->IsVisible();
}

}  // namespace ash
