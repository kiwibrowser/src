// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/host_pairing_screen_view.h"
#include "components/login/screens/screen_context.h"
#include "components/pairing/host_pairing_controller.h"

namespace chromeos {

class HostPairingScreen
    : public BaseScreen,
      public pairing_chromeos::HostPairingController::Observer,
      public HostPairingScreenView::Delegate,
      public EnterpriseEnrollmentHelper::EnrollmentStatusConsumer {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when a configuration has been received, and should be applied to
    // this device.
    virtual void ConfigureHostRequested(bool accepted_eula,
                                        const std::string& lang,
                                        const std::string& timezone,
                                        bool send_reports,
                                        const std::string& keyboard_layout) = 0;

    // Called when a network configuration has been received, and should be
    // used on this device.
    virtual void AddNetworkRequested(const std::string& onc_spec) = 0;

    // Called when a reboot message has been received, and should reboot this
    // device.
    virtual void RebootHostRequested() = 0;
  };

  HostPairingScreen(BaseScreenDelegate* base_screen_delegate,
                    Delegate* delegate,
                    HostPairingScreenView* view,
                    pairing_chromeos::HostPairingController* remora_controller);
  ~HostPairingScreen() override;

 private:
  typedef pairing_chromeos::HostPairingController::Stage Stage;

  void CommitContextChanges();

  // Overridden from BaseScreen:
  void Show() override;
  void Hide() override;

  // pairing_chromeos::HostPairingController::Observer:
  void PairingStageChanged(Stage new_stage) override;
  void ConfigureHostRequested(bool accepted_eula,
                              const std::string& lang,
                              const std::string& timezone,
                              bool send_reports,
                              const std::string& keyboard_layout) override;
  void AddNetworkRequested(const std::string& onc_spec) override;
  void EnrollHostRequested(const std::string& auth_token) override;
  void RebootHostRequested() override;

  // Overridden from ControllerPairingView::Delegate:
  void OnViewDestroyed(HostPairingScreenView* view) override;

  // Overridden from EnterpriseEnrollmentHelper::EnrollmentStatusConsumer:
  void OnAuthError(const GoogleServiceAuthError& error) override;
  void OnMultipleLicensesAvailable(
      const EnrollmentLicenseMap& licenses) override;
  void OnEnrollmentError(policy::EnrollmentStatus status) override;
  void OnOtherError(EnterpriseEnrollmentHelper::OtherError error) override;
  void OnDeviceEnrolled(const std::string& additional_token) override;
  void OnDeviceAttributeUploadCompleted(bool success) override;
  void OnDeviceAttributeUpdatePermission(bool granted) override;

  // Used as a callback for EnterpriseEnrollmentHelper::ClearAuth.
  void OnAuthCleared();
  void OnAnyEnrollmentError();

  Delegate* delegate_ = nullptr;

  HostPairingScreenView* view_ = nullptr;

  // Controller performing pairing. Owned by the wizard controller.
  pairing_chromeos::HostPairingController* remora_controller_ = nullptr;

  std::unique_ptr<EnterpriseEnrollmentHelper> enrollment_helper_;

  // Describes the error code of an enrollment operation. For the format, see
  // the definition of |error_code_| in bluetooth_host_pairing_controller.h.
  int enrollment_error_code_ = 0;
  std::string enrollment_error_string_;

  // Current stage of pairing process.
  Stage current_stage_ = pairing_chromeos::HostPairingController::STAGE_NONE;

  base::WeakPtrFactory<HostPairingScreen> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HostPairingScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HOST_PAIRING_SCREEN_H_
