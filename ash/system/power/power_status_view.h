// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_STATUS_VIEW_H_
#define ASH_SYSTEM_POWER_POWER_STATUS_VIEW_H_

#include "ash/ash_export.h"
#include "ash/system/power/power_status.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/views/view.h"

namespace views {
class Label;
}

namespace ash {

class ASH_EXPORT PowerStatusView : public views::View,
                                   public PowerStatus::Observer {
 public:
  PowerStatusView();
  ~PowerStatusView() override;

  // views::View:
  void Layout() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // PowerStatus::Observer:
  void OnPowerStatusChanged() override;

 private:
  friend class PowerStatusViewTest;

  void UpdateText();

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;

  views::Label* percentage_label_;
  views::Label* separator_label_;
  views::Label* time_status_label_;

  base::string16 accessible_name_;

  DISALLOW_COPY_AND_ASSIGN(PowerStatusView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_STATUS_VIEW_H_
