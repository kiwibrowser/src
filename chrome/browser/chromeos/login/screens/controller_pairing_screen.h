// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_CONTROLLER_PAIRING_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_CONTROLLER_PAIRING_SCREEN_H_

#include "base/macros.h"

#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/controller_pairing_screen_view.h"
#include "components/login/screens/screen_context.h"
#include "components/pairing/controller_pairing_controller.h"

namespace chromeos {

class ControllerPairingScreen
    : public BaseScreen,
      public pairing_chromeos::ControllerPairingController::Observer,
      public ControllerPairingScreenView::Delegate {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Set remora network from shark.
    virtual void SetHostNetwork() = 0;

    // Set remora configuration from shark.
    virtual void SetHostConfiguration() = 0;
  };

  ControllerPairingScreen(
      BaseScreenDelegate* base_screen_delegate,
      Delegate* delegate,
      ControllerPairingScreenView* view,
      pairing_chromeos::ControllerPairingController* shark_controller);
  ~ControllerPairingScreen() override;

 private:
  typedef pairing_chromeos::ControllerPairingController::Stage Stage;

  void CommitContextChanges();
  bool ExpectStageIs(Stage stage) const;

  // Overridden from BaseScreen:
  void Show() override;
  void Hide() override;

  // Overridden from pairing_chromeos::ControllerPairingController::Observer:
  void PairingStageChanged(Stage new_stage) override;
  void DiscoveredDevicesListChanged() override;

  // Overridden from ControllerPairingView::Delegate:
  void OnViewDestroyed(ControllerPairingScreenView* view) override;
  void OnScreenContextChanged(const base::DictionaryValue& diff) override;
  void OnUserActed(const std::string& action) override;

  Delegate* delegate_;

  ControllerPairingScreenView* view_;

  // Controller performing pairing. Owned by the wizard controller.
  pairing_chromeos::ControllerPairingController* shark_controller_;

  // Current stage of pairing process.
  Stage current_stage_;

  // If this one is |false| first device in device list will be preselected on
  // next device list update.
  bool device_preselected_;

  DISALLOW_COPY_AND_ASSIGN(ControllerPairingScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_CONTROLLER_PAIRING_SCREEN_H_
