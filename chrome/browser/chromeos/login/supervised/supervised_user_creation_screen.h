// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_SCREEN_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/camera_presence_notifier.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_controller.h"
#include "chrome/browser/image_decoder.h"
#include "chrome/browser/ui/webui/chromeos/login/supervised_user_creation_screen_handler.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"
#include "components/login/secure_module_util_chromeos.h"
#include "ui/gfx/image/image_skia.h"

class Profile;

namespace chromeos {

class ErrorScreensHistogramHelper;
class NetworkState;
class ScreenManager;

// Class that controls screen showing ui for supervised user creation.
class SupervisedUserCreationScreen
    : public BaseScreen,
      public SupervisedUserCreationScreenHandler::Delegate,
      public SupervisedUserCreationController::StatusConsumer,
      public ImageDecoder::ImageRequest,
      public NetworkPortalDetector::Observer,
      public CameraPresenceNotifier::Observer {
 public:
  SupervisedUserCreationScreen(BaseScreenDelegate* base_screen_delegate,
                               SupervisedUserCreationScreenHandler* view);
  ~SupervisedUserCreationScreen() override;

  static SupervisedUserCreationScreen* Get(ScreenManager* manager);

  // Makes screen to show message about inconsistency in manager login flow
  // (e.g. password change detected, invalid OAuth token, etc).
  // Called when manager user is successfully authenticated, so ui elements
  // should result in forced logout.
  void ShowManagerInconsistentStateErrorScreen();

  // Called when authentication fails for manager with provided password.
  // Displays wrong password message on manager selection screen.
  void OnManagerLoginFailure();

  // Called when manager is successfully authenticated and account is in
  // consistent state.
  void OnManagerFullyAuthenticated(Profile* manager_profile);

  // Called when manager is successfully authenticated against cryptohome, but
  // OAUTH token validation hasn't completed yet.
  // Results in spinner indicating that creation is in process.
  void OnManagerCryptohomeAuthenticated();

  // Shows initial screen where managed user name/password are defined and
  // manager is selected.
  void ShowInitialScreen();

  // CameraPresenceNotifier::Observer implementation:
  void OnCameraPresenceCheckDone(bool is_camera_present) override;

  // BaseScreen implementation:
  void Show() override;
  void Hide() override;

  // SupervisedUserCreationScreenHandler::Delegate implementation:
  void OnViewDestroyed(SupervisedUserCreationScreenHandler* view) override;
  void CreateSupervisedUser(
      const base::string16& display_name,
      const std::string& supervised_user_password) override;
  void ImportSupervisedUser(const std::string& user_id) override;
  void ImportSupervisedUserWithPassword(const std::string& user_id,
                                        const std::string& password) override;
  void AuthenticateManager(const AccountId& manager_id,
                           const std::string& manager_password) override;
  void AbortFlow() override;
  void FinishFlow() override;
  void HideFlow() override;
  bool FindUserByDisplayName(const base::string16& display_name,
                             std::string* out_id) const override;
  void OnPageSelected(const std::string& page) override;

  // SupervisedUserController::StatusConsumer overrides.
  void OnCreationError(
      SupervisedUserCreationController::ErrorCode code) override;
  void OnCreationTimeout() override;
  void OnCreationSuccess() override;
  void OnLongCreationWarning() override;

  // NetworkPortalDetector::Observer implementation:
  void OnPortalDetectionCompleted(
      const NetworkState* network,
      const NetworkPortalDetector::CaptivePortalState& state) override;

  // TODO(antrim) : this is an explicit code duplications with UserImageScreen.
  // It should be removed by issue 251179.

  // SupervisedUserCreationScreenHandler::Delegate (image) implementation:
  void OnPhotoTaken(const std::string& raw_data) override;
  void OnImageSelected(const std::string& image_type,
                       const std::string& image_url) override;
  void OnImageAccepted() override;
  // ImageDecoder::ImageRequest overrides:
  void OnImageDecoded(const SkBitmap& decoded_image) override;
  void OnDecodeImageFailed() override;

 private:
  void ApplyPicture();
  void UpdateSecureModuleMessages(::login::SecureModuleUsed secure_module_used);

  SupervisedUserCreationScreenHandler* view_;

  std::unique_ptr<SupervisedUserCreationController> controller_;
  std::unique_ptr<base::DictionaryValue> existing_users_;

  bool on_error_screen_;
  bool manager_signin_in_progress_;
  std::string last_page_;

  gfx::ImageSkia user_photo_;
  bool apply_photo_after_decoding_;
  int selected_image_;

  std::unique_ptr<ErrorScreensHistogramHelper> histogram_helper_;

  base::WeakPtrFactory<SupervisedUserCreationScreen> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SupervisedUserCreationScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SUPERVISED_SUPERVISED_USER_CREATION_SCREEN_H_
