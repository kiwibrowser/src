// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_DATE_TRAY_SYSTEM_INFO_H_
#define ASH_SYSTEM_DATE_TRAY_SYSTEM_INFO_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/login_status.h"
#include "ash/system/date/clock_observer.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"

namespace views {
class Label;
}

namespace ash {
class SystemInfoDefaultView;

namespace tray {
class TimeView;
}

// The bottom row of the system menu. The default view shows the current date
// and power status. The tray view shows the current time.
class ASH_EXPORT TraySystemInfo : public SystemTrayItem {
 public:
  explicit TraySystemInfo(SystemTray* system_tray);
  ~TraySystemInfo() override;

  const tray::TimeView* GetTimeTrayForTesting() const;
  const SystemInfoDefaultView* GetDefaultViewForTesting() const;
  views::View* CreateDefaultViewForTesting(LoginStatus status);

 private:
  // SystemTrayItem:
  views::View* CreateTrayView(LoginStatus status) override;
  views::View* CreateDefaultView(LoginStatus status) override;
  void OnTrayViewDestroyed() override;
  void OnDefaultViewDestroyed() override;
  void UpdateAfterShelfAlignmentChange() override;

  void SetupLabelForTimeTray(views::Label* label);

  tray::TimeView* tray_view_;
  SystemInfoDefaultView* default_view_;

  DISALLOW_COPY_AND_ASSIGN(TraySystemInfo);
};

}  // namespace ash

#endif  // ASH_SYSTEM_DATE_TRAY_SYSTEM_INFO_H_
