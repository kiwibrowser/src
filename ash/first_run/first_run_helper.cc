// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/first_run/first_run_helper.h"

#include "ash/first_run/desktop_cleaner.h"
#include "ash/public/cpp/ash_features.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/app_list_button.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_bubble.h"
#include "ash/system/unified/unified_system_tray.h"
#include "base/logging.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

namespace ash {
namespace {

}  // namespace

FirstRunHelper::FirstRunHelper() = default;

FirstRunHelper::~FirstRunHelper() = default;

void FirstRunHelper::BindRequest(mojom::FirstRunHelperRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void FirstRunHelper::Start(mojom::FirstRunHelperClientPtr client) {
  client_ = std::move(client);
  cleaner_ = std::make_unique<DesktopCleaner>();
  Shell::Get()->session_controller()->AddObserver(this);
}

void FirstRunHelper::Stop() {
  Shell::Get()->session_controller()->RemoveObserver(this);
  // Ensure the tray is closed.
  CloseTrayBubble();
  cleaner_.reset();
}

void FirstRunHelper::GetAppListButtonBounds(GetAppListButtonBoundsCallback cb) {
  Shelf* shelf = Shelf::ForWindow(Shell::GetPrimaryRootWindow());
  AppListButton* app_button = shelf->shelf_widget()->GetAppListButton();
  std::move(cb).Run(app_button->GetBoundsInScreen());
}

void FirstRunHelper::OpenTrayBubble(OpenTrayBubbleCallback cb) {
  if (features::IsSystemTrayUnifiedEnabled()) {
    UnifiedSystemTray* tray = Shell::Get()
                                  ->GetPrimaryRootWindowController()
                                  ->GetStatusAreaWidget()
                                  ->unified_system_tray();
    tray->ShowBubble(false /* show_by_click */);
    std::move(cb).Run(tray->GetBubbleBoundsInScreen());
  } else {
    SystemTray* tray = Shell::Get()->GetPrimarySystemTray();
    tray->ShowPersistentDefaultView();
    views::View* bubble = tray->GetSystemBubble()->bubble_view();
    std::move(cb).Run(bubble->GetBoundsInScreen());
  }
}

void FirstRunHelper::CloseTrayBubble() {
  if (features::IsSystemTrayUnifiedEnabled()) {
    Shell::Get()
        ->GetPrimaryRootWindowController()
        ->GetStatusAreaWidget()
        ->unified_system_tray()
        ->CloseBubble();
  } else {
    Shell::Get()->GetPrimarySystemTray()->CloseBubble();
  }
}

void FirstRunHelper::GetHelpButtonBounds(GetHelpButtonBoundsCallback cb) {
  if (features::IsSystemTrayUnifiedEnabled()) {
    std::move(cb).Run(gfx::Rect());
    return;
  }
  SystemTray* tray = Shell::Get()->GetPrimarySystemTray();
  views::View* help_button = tray->GetHelpButtonView();
  // |help_button| could be null if the tray isn't open.
  if (!help_button) {
    std::move(cb).Run(gfx::Rect());
    return;
  }
  std::move(cb).Run(help_button->GetBoundsInScreen());
}

void FirstRunHelper::OnLockStateChanged(bool locked) {
  Cancel();
}

void FirstRunHelper::OnChromeTerminating() {
  Cancel();
}

void FirstRunHelper::FlushForTesting() {
  client_.FlushForTesting();
}

void FirstRunHelper::Cancel() {
  if (client_)
    client_->OnCancelled();
}

}  // namespace ash
