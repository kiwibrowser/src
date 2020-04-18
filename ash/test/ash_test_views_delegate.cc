// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/ash_test_views_delegate.h"

#include "ash/shell.h"
#include "ash/test/ash_test_helper.h"

namespace ash {

AshTestViewsDelegate::AshTestViewsDelegate() = default;

AshTestViewsDelegate::~AshTestViewsDelegate() = default;

void AshTestViewsDelegate::OnBeforeWidgetInit(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  TestViewsDelegate::OnBeforeWidgetInit(params, delegate);

  if (!params->parent && !params->context && ash::Shell::HasInstance())
    params->context = Shell::GetRootWindowForNewWindows();
}

void AshTestViewsDelegate::NotifyAccessibilityEvent(
    views::View* view,
    ax::mojom::Event event_type) {
  TestViewsDelegate::NotifyAccessibilityEvent(view, event_type);

  if (test_accessibility_event_delegate_) {
    test_accessibility_event_delegate_->NotifyAccessibilityEvent(view,
                                                                 event_type);
  }
}

views::TestViewsDelegate::ProcessMenuAcceleratorResult
AshTestViewsDelegate::ProcessAcceleratorWhileMenuShowing(
    const ui::Accelerator& accelerator) {
  if (accelerator == close_menu_accelerator_)
    return ProcessMenuAcceleratorResult::CLOSE_MENU;

  return ProcessMenuAcceleratorResult::LEAVE_MENU_OPEN;
}

}  // namespace ash
