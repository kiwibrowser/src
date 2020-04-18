// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRACING_H_
#define ASH_SYSTEM_TRAY_TRACING_H_

#include "ash/ash_export.h"
#include "ash/system/model/tracing_model.h"
#include "ash/system/tray/tray_image_item.h"
#include "base/macros.h"

namespace views {
class View;
}

namespace ash {

// This is the item that displays when users enable performance tracing at
// chrome://slow.  It alerts them that this mode is running, and provides an
// easy way to open the page to disable it.
class TrayTracing : public TrayImageItem, public TracingObserver {
 public:
  explicit TrayTracing(SystemTray* system_tray);
  ~TrayTracing() override;

 private:
  void UpdateTrayIcon();

  // Overridden from TrayImageItem.
  bool GetInitialVisibility() override;
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;
  void OnDetailedViewDestroyed() override;

  // Overridden from TracingObserver.
  void OnTracingModeChanged() override;

  DISALLOW_COPY_AND_ASSIGN(TrayTracing);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TRACING_H_
