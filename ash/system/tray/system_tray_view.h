// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_SYSTEM_TRAY_VIEW_H_
#define ASH_SYSTEM_TRAY_SYSTEM_TRAY_VIEW_H_

#include "ash/login_status.h"
#include "ash/system/tray/system_tray_item.h"
#include "ui/views/view.h"

namespace ash {

class SystemTrayView : public views::View {
 public:
  enum SystemTrayType { SYSTEM_TRAY_TYPE_DEFAULT, SYSTEM_TRAY_TYPE_DETAILED };

  SystemTrayView(SystemTray* system_tray,
                 SystemTrayType system_tray_type,
                 const std::vector<ash::SystemTrayItem*>& items);
  ~SystemTrayView() override;

  bool CreateItemViews(LoginStatus login_status);

  void DestroyItemViews();

  // Records metrics for visible system menu rows. Only implemented for the
  // SYSTEM_TRAY_TYPE_DEFAULT SystemTrayType.
  void RecordVisibleRowMetrics();

  SystemTrayType system_tray_type() const { return system_tray_type_; }
  void set_system_tray_type(SystemTrayType system_tray_type) {
    system_tray_type_ = system_tray_type;
  }

  const std::vector<ash::SystemTrayItem*>& items() const { return items_; }
  void set_items(const std::vector<ash::SystemTrayItem*>& items) {
    items_ = items;
  }

 private:
  // Tracks the views created in the last call to CreateItemViews().
  std::map<SystemTrayItem::UmaType, views::View*> tray_item_view_map_;

  std::unique_ptr<ui::EventHandler> time_to_click_recorder_;
  std::unique_ptr<ui::EventHandler> interacted_by_tap_recorder_;

  std::vector<ash::SystemTrayItem*> items_;

  SystemTrayType system_tray_type_;

  DISALLOW_COPY_AND_ASSIGN(SystemTrayView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_SYSTEM_TRAY_VIEW_H_
