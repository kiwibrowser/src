// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_NOTIFICATION_COUNTER_VIEW_H_
#define ASH_SYSTEM_UNIFIED_NOTIFICATION_COUNTER_VIEW_H_

#include "ash/system/tray/tray_item_view.h"
#include "base/macros.h"

namespace ash {

// A notification counter view in UnifiedSystemTray button.
class NotificationCounterView : public TrayItemView {
 public:
  NotificationCounterView();
  ~NotificationCounterView() override;

  void Update();

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationCounterView);
};

// A do-not-distrub icon view in UnifiedSystemTray button.
class QuietModeView : public TrayItemView {
 public:
  QuietModeView();
  ~QuietModeView() override;

  void Update();

 private:
  DISALLOW_COPY_AND_ASSIGN(QuietModeView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_NOTIFICATION_COUNTER_VIEW_H_
