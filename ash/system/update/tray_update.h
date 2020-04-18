// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UPDATE_TRAY_UPDATE_H_
#define ASH_SYSTEM_UPDATE_TRAY_UPDATE_H_

#include "ash/ash_export.h"
#include "ash/system/model/update_model.h"
#include "ash/system/tray/tray_image_item.h"
#include "base/macros.h"
#include "base/strings/string16.h"

namespace views {
class Label;
class View;
}

namespace ash {

// The system update tray item. The tray icon stays visible once an update
// notification is received. The icon only disappears after a reboot to apply
// the update. Exported for test.
class ASH_EXPORT TrayUpdate : public TrayImageItem, public UpdateObserver {
 public:
  explicit TrayUpdate(SystemTray* system_tray);
  ~TrayUpdate() override;

  // Expose label information for testing.
  views::Label* GetLabelForTesting();

  // Overridden from UpdateObserver.
  void OnUpdateAvailable() override;

 private:
  class UpdateView;

  // Returns true if the tray view and default view should be visible.
  bool ShouldShowUpdate() const;

  // Overridden from TrayImageItem.
  bool GetInitialVisibility() override;
  views::View* CreateTrayView(LoginStatus status) override;
  views::View* CreateDefaultView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;

  UpdateModel* const model_;
  UpdateView* update_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TrayUpdate);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UPDATE_TRAY_UPDATE_H_
