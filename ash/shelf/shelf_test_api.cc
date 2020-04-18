// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_test_api.h"

#include "ash/root_window_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ash/shell.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace ash {

ShelfTestApi::ShelfTestApi(Shelf* shelf) : shelf_(shelf) {}

ShelfTestApi::~ShelfTestApi() = default;

// static
void ShelfTestApi::BindRequest(mojom::ShelfTestApiRequest request) {
  Shelf* shelf = Shell::Get()->GetPrimaryRootWindowController()->shelf();
  mojo::MakeStrongBinding(std::make_unique<ShelfTestApi>(shelf),
                          std::move(request));
}

void ShelfTestApi::IsVisible(IsVisibleCallback cb) {
  std::move(cb).Run(shelf_->shelf_layout_manager()->IsVisible());
}

void ShelfTestApi::UpdateVisibility(UpdateVisibilityCallback cb) {
  shelf_->shelf_layout_manager()->UpdateVisibilityState();
  std::move(cb).Run();
}

void ShelfTestApi::HasOverlappingWindow(HasOverlappingWindowCallback cb) {
  std::move(cb).Run(shelf_->shelf_layout_manager()->window_overlaps_shelf());
}

void ShelfTestApi::IsAlignmentBottomLocked(IsAlignmentBottomLockedCallback cb) {
  std::move(cb).Run(shelf_->alignment() == SHELF_ALIGNMENT_BOTTOM_LOCKED);
}

}  // namespace ash
