// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_BRIGHTNESS_TRAY_BRIGHTNESS_H_
#define ASH_SYSTEM_BRIGHTNESS_TRAY_BRIGHTNESS_H_

#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "chromeos/dbus/power_manager_client.h"

namespace ash {
namespace tray {

class BrightnessView;

// We don't let the screen brightness go lower than this when it's being
// adjusted via the slider.  Otherwise, if the user doesn't know about the
// brightness keys, they may turn the backlight off and not know how to turn it
// back on.
constexpr double kMinBrightnessPercent = 5.0;

}  // namespace tray

class ASH_EXPORT TrayBrightness
    : public SystemTrayItem,
      public chromeos::PowerManagerClient::Observer {
 public:
  explicit TrayBrightness(SystemTray* system_tray);
  ~TrayBrightness() override;

 private:
  friend class TrayBrightnessTest;

  // Sends a request to get the current screen brightness so |current_percent_|
  // can be initialized.
  void GetInitialBrightness();

  // Updates |current_percent_| with the initial brightness requested by
  // GetInitialBrightness(), if we haven't seen the brightness already in the
  // meantime.
  void HandleInitialBrightness(base::Optional<double> percent);

  // Overridden from SystemTrayItem.
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;
  void OnDetailedViewDestroyed() override;
  void UpdateAfterLoginStatusChange(LoginStatus status) override;
  bool ShouldShowShelf() const override;

  // Overriden from PowerManagerClient::Observer.
  void ScreenBrightnessChanged(
      const power_manager::BacklightBrightnessChange& change) override;

  void HandleBrightnessChanged(double percent, bool user_initiated);

  tray::BrightnessView* brightness_view_;

  // Brightness level in the range [0.0, 100.0] that we've heard about most
  // recently.
  double current_percent_;

  // Has |current_percent_| been initialized?
  bool got_current_percent_;

  base::WeakPtrFactory<TrayBrightness> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrayBrightness);
};

}  // namespace ash

#endif  // ASH_SYSTEM_BRIGHTNESS_TRAY_BRIGHTNESS_H_
