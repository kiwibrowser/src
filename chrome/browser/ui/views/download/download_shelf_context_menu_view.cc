// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/download/download_shelf_context_menu_view.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "chrome/browser/download/download_item_model.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/page_navigator.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/menu/menu_runner.h"

DownloadShelfContextMenuView::DownloadShelfContextMenuView(
    DownloadItemView* download_item_view)
    : DownloadShelfContextMenu(download_item_view->download()),
      download_item_view_(download_item_view) {}

DownloadShelfContextMenuView::~DownloadShelfContextMenuView() {}

void DownloadShelfContextMenuView::Run(
    views::Widget* parent_widget,
    const gfx::Rect& rect,
    ui::MenuSourceType source_type,
    const base::Closure& on_menu_closed_callback) {
  ui::MenuModel* menu_model = GetMenuModel();
  // Run() should not be getting called if the DownloadItem was destroyed.
  DCHECK(menu_model);

  menu_runner_.reset(new views::MenuRunner(
      menu_model,
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU,
      base::Bind(&DownloadShelfContextMenuView::OnMenuClosed,
                 base::Unretained(this), on_menu_closed_callback)));

  // The menu's alignment is determined based on the UI layout.
  views::MenuAnchorPosition position;
  if (base::i18n::IsRTL())
    position = views::MENU_ANCHOR_TOPRIGHT;
  else
    position = views::MENU_ANCHOR_TOPLEFT;

  menu_runner_->RunMenuAt(parent_widget, NULL, rect, position, source_type);
}

void DownloadShelfContextMenuView::OnMenuClosed(
    const base::Closure& on_menu_closed_callback) {
  close_time_ = base::TimeTicks::Now();

  // This must be run before clearing |menu_runner_| who owns the reference.
  if (!on_menu_closed_callback.is_null())
    on_menu_closed_callback.Run();

  menu_runner_.reset();
}

void DownloadShelfContextMenuView::ExecuteCommand(int command_id,
                                                  int event_flags) {
  DownloadCommands::Command command =
      static_cast<DownloadCommands::Command>(command_id);
  DCHECK_NE(command, DownloadCommands::DISCARD);

  if (command == DownloadCommands::KEEP) {
    download_item_view_->MaybeSubmitDownloadToFeedbackService(
        DownloadCommands::KEEP);
  } else {
    DownloadShelfContextMenu::ExecuteCommand(command_id, event_flags);
  }
}
