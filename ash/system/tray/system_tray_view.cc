// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/system_tray_view.h"

#include "ash/shell.h"
#include "ash/system/tray/interacted_by_tap_recorder.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/metrics/histogram_macros.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

namespace {

class BottomAlignedBoxLayout : public views::BoxLayout {
 public:
  explicit BottomAlignedBoxLayout() : BoxLayout(BoxLayout::kVertical) {
    SetDefaultFlex(1);
  }

  ~BottomAlignedBoxLayout() override = default;

 private:
  void Layout(views::View* host) override {
    if (host->height() >= host->GetPreferredSize().height()) {
      BoxLayout::Layout(host);
      return;
    }

    int consumed_height = 0;
    for (int i = host->child_count() - 1;
         i >= 0 && consumed_height < host->height(); --i) {
      views::View* child = host->child_at(i);
      if (!child->visible())
        continue;
      gfx::Size size = child->GetPreferredSize();
      child->SetBounds(0, host->height() - consumed_height - size.height(),
                       host->width(), size.height());
      consumed_height += size.height();
    }
  }

  DISALLOW_COPY_AND_ASSIGN(BottomAlignedBoxLayout);
};

}  // anonymous namespace

SystemTrayView::SystemTrayView(SystemTray* system_tray,
                               SystemTrayType system_tray_type,
                               const std::vector<ash::SystemTrayItem*>& items)
    : time_to_click_recorder_(
          std::make_unique<TimeToClickRecorder>(system_tray, this)),
      interacted_by_tap_recorder_(
          std::make_unique<InteractedByTapRecorder>(this)),
      items_(items),
      system_tray_type_(system_tray_type) {
  SetLayoutManager(std::make_unique<BottomAlignedBoxLayout>());
}

SystemTrayView::~SystemTrayView() {
  DestroyItemViews();
}

bool SystemTrayView::CreateItemViews(LoginStatus login_status) {
  tray_item_view_map_.clear();

  // If a system modal dialog is present, create the same tray as
  // in locked state.
  if (Shell::IsSystemModalWindowOpen() &&
      login_status != LoginStatus::NOT_LOGGED_IN) {
    login_status = LoginStatus::LOCKED;
  }

  views::View* focus_view = nullptr;
  for (ash::SystemTrayItem* const it : items_) {
    views::View* item_view = nullptr;
    switch (system_tray_type_) {
      case SYSTEM_TRAY_TYPE_DEFAULT:
        item_view = it->CreateDefaultView(login_status);
        if (it->restore_focus()) {
          focus_view = it->GetItemToRestoreFocusTo()
                           ? it->GetItemToRestoreFocusTo()
                           : item_view;
          focus_view = item_view;
        }
        break;
      case SYSTEM_TRAY_TYPE_DETAILED:
        item_view = it->CreateDetailedView(login_status);
        break;
    }
    if (item_view) {
      AddChildView(item_view);
      tray_item_view_map_[it->uma_type()] = item_view;
    }
  }

  if (focus_view) {
    focus_view->RequestFocus();
    return true;
  }
  return false;
}

void SystemTrayView::DestroyItemViews() {
  for (ash::SystemTrayItem* const it : items_) {
    switch (system_tray_type_) {
      case SYSTEM_TRAY_TYPE_DEFAULT:
        it->OnDefaultViewDestroyed();
        break;
      case SYSTEM_TRAY_TYPE_DETAILED:
        it->OnDetailedViewDestroyed();
        break;
    }
  }
}

void SystemTrayView::RecordVisibleRowMetrics() {
  if (system_tray_type_ != SYSTEM_TRAY_TYPE_DEFAULT)
    return;

  for (const std::pair<SystemTrayItem::UmaType, views::View*>& pair :
       tray_item_view_map_) {
    if (pair.second->visible() &&
        pair.first != SystemTrayItem::UMA_NOT_RECORDED) {
      UMA_HISTOGRAM_ENUMERATION("Ash.SystemMenu.DefaultView.VisibleRows",
                                pair.first, SystemTrayItem::UMA_COUNT);
    }
  }
}

}  // namespace ash
