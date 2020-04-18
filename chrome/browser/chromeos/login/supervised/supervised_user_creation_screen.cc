// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_screen.h"

#include <memory>
#include <utility>

#include "ash/shell.h"
#include "base/values.h"
#include "chrome/browser/chromeos/camera_detector.h"
#include "chrome/browser/chromeos/login/error_screens_histogram_helper.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/screen_manager.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/error_screen.h"
#include "chrome/browser/chromeos/login/screens/network_error.h"
#include "chrome/browser/chromeos/login/signin_specifics.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_authentication.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_controller.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_controller_new.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_flow.h"
#include "chrome/browser/chromeos/login/users/avatar/user_image_manager.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/supervised_user_manager.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/login/auth/key.h"
#include "chromeos/login/auth/user_context.h"
#include "chromeos/network/network_state.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_image/user_image.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image_skia.h"

namespace chromeos {

namespace {

const char kNameOfIntroScreen[] = "intro";
const char kNameOfNewUserParametersScreen[] = "username";

void ConfigureErrorScreen(
    ErrorScreen* screen,
    const NetworkState* network,
    const NetworkPortalDetector::CaptivePortalStatus status) {
  switch (status) {
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_UNKNOWN:
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE:
      NOTREACHED();
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_OFFLINE:
      screen->SetErrorState(NetworkError::ERROR_STATE_OFFLINE, std::string());
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL:
      screen->SetErrorState(NetworkError::ERROR_STATE_PORTAL,
                            network ? network->name() : std::string());
      screen->FixCaptivePortal();
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PROXY_AUTH_REQUIRED:
      screen->SetErrorState(NetworkError::ERROR_STATE_PROXY, std::string());
      break;
    default:
      NOTREACHED();
      break;
  }
}

}  // namespace

// static
SupervisedUserCreationScreen* SupervisedUserCreationScreen::Get(
    ScreenManager* manager) {
  return static_cast<SupervisedUserCreationScreen*>(
      manager->GetScreen(OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW));
}

SupervisedUserCreationScreen::SupervisedUserCreationScreen(
    BaseScreenDelegate* base_screen_delegate,
    SupervisedUserCreationScreenHandler* view)
    : BaseScreen(base_screen_delegate,
                 OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW),
      view_(view),
      on_error_screen_(false),
      manager_signin_in_progress_(false),
      last_page_(kNameOfIntroScreen),
      apply_photo_after_decoding_(false),
      selected_image_(0),
      histogram_helper_(new ErrorScreensHistogramHelper("Supervised")),
      weak_factory_(this) {
  DCHECK(view_);
  if (view_)
    view_->SetDelegate(this);
}

SupervisedUserCreationScreen::~SupervisedUserCreationScreen() {
  CameraPresenceNotifier::GetInstance()->RemoveObserver(this);
  if (view_)
    view_->SetDelegate(NULL);
  network_portal_detector::GetInstance()->RemoveObserver(this);
}

void SupervisedUserCreationScreen::Show() {
  CameraPresenceNotifier::GetInstance()->AddObserver(this);
  if (view_) {
    view_->Show();
    // TODO(antrim) : temorary hack (until upcoming hackaton). Should be
    // removed once we have screens reworked.
    if (on_error_screen_)
      view_->ShowPage(last_page_);
    else
      view_->ShowIntroPage();
  }

  if (!on_error_screen_)
    network_portal_detector::GetInstance()->AddAndFireObserver(this);
  on_error_screen_ = false;
  histogram_helper_->OnScreenShow();
}

void SupervisedUserCreationScreen::OnPageSelected(const std::string& page) {
  last_page_ = page;
}

void SupervisedUserCreationScreen::OnPortalDetectionCompleted(
    const NetworkState* network,
    const NetworkPortalDetector::CaptivePortalState& state) {
  if (state.status == NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE) {
    get_base_screen_delegate()->HideErrorScreen(this);
    histogram_helper_->OnErrorHide();
  } else if (state.status !=
             NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_UNKNOWN) {
    on_error_screen_ = true;
    ErrorScreen* screen = get_base_screen_delegate()->GetErrorScreen();
    ConfigureErrorScreen(screen, network, state.status);
    screen->SetUIState(NetworkError::UI_STATE_SUPERVISED);
    get_base_screen_delegate()->ShowErrorScreen();
    histogram_helper_->OnErrorShow(screen->GetErrorState());
  }
}

void SupervisedUserCreationScreen::ShowManagerInconsistentStateErrorScreen() {
  manager_signin_in_progress_ = false;
  if (!view_)
    return;
  view_->ShowErrorPage(
      l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_MANAGER_INCONSISTENT_STATE_TITLE),
      l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_MANAGER_INCONSISTENT_STATE),
      l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_MANAGER_INCONSISTENT_STATE_BUTTON));
}

void SupervisedUserCreationScreen::ShowInitialScreen() {
  if (view_)
    view_->ShowIntroPage();
}

void SupervisedUserCreationScreen::Hide() {
  CameraPresenceNotifier::GetInstance()->RemoveObserver(this);
  if (view_)
    view_->Hide();
  if (!on_error_screen_)
    network_portal_detector::GetInstance()->RemoveObserver(this);
}

void SupervisedUserCreationScreen::AbortFlow() {
  DBusThreadManager::Get()
      ->GetSessionManagerClient()
      ->NotifySupervisedUserCreationFinished();
  controller_->CancelCreation();
}

void SupervisedUserCreationScreen::FinishFlow() {
  DBusThreadManager::Get()
      ->GetSessionManagerClient()
      ->NotifySupervisedUserCreationFinished();
  controller_->FinishCreation();
}

void SupervisedUserCreationScreen::HideFlow() {
  Hide();
}

void SupervisedUserCreationScreen::AuthenticateManager(
    const AccountId& manager_id,
    const std::string& manager_password) {
  if (manager_signin_in_progress_)
    return;
  manager_signin_in_progress_ = true;

  UserFlow* flow = new SupervisedUserCreationFlow(manager_id);
  ChromeUserManager::Get()->SetUserFlow(manager_id, flow);

  // Make sure no two controllers exist at the same time.
  controller_.reset();

  controller_.reset(new SupervisedUserCreationControllerNew(this, manager_id));

  const user_manager::User* const manager =
      user_manager::UserManager::Get()->FindUser(manager_id);
  // Tests depend on the ability to create users on-demand, so we cannot
  // require manager to exist here.
  UserContext user_context =
      manager
          ? UserContext(*manager)
          : UserContext(user_manager::UserType::USER_TYPE_REGULAR, manager_id);
  user_context.SetKey(Key(manager_password));
  ExistingUserController::current_controller()->Login(user_context,
                                                      SigninSpecifics());
}

void SupervisedUserCreationScreen::CreateSupervisedUser(
    const base::string16& display_name,
    const std::string& supervised_user_password) {
  NOTREACHED();
}

void SupervisedUserCreationScreen::ImportSupervisedUser(
    const std::string& user_id) {
  NOTREACHED();
}

// TODO(antrim): Code duplication with previous method will be removed once
// password sync is implemented.
void SupervisedUserCreationScreen::ImportSupervisedUserWithPassword(
    const std::string& user_id,
    const std::string& password) {
  NOTREACHED();
}

void SupervisedUserCreationScreen::OnManagerLoginFailure() {
  manager_signin_in_progress_ = false;
  if (view_)
    view_->ShowManagerPasswordError();
}

void SupervisedUserCreationScreen::OnManagerFullyAuthenticated(
    Profile* manager_profile) {
  DBusThreadManager::Get()
      ->GetSessionManagerClient()
      ->NotifySupervisedUserCreationStarted();
  manager_signin_in_progress_ = false;
  DCHECK(controller_.get());

  controller_->SetManagerProfile(manager_profile);
  if (view_)
    view_->ShowUsernamePage();

  last_page_ = kNameOfNewUserParametersScreen;
}

void SupervisedUserCreationScreen::OnManagerCryptohomeAuthenticated() {
  if (view_) {
    view_->ShowStatusMessage(
        true /* progress */,
        l10n_util::GetStringUTF16(
            IDS_CREATE_SUPERVISED_USER_CREATION_AUTH_PROGRESS_MESSAGE));
  }
}

void SupervisedUserCreationScreen::OnViewDestroyed(
    SupervisedUserCreationScreenHandler* view) {
  if (view_ == view)
    view_ = NULL;
}

void SupervisedUserCreationScreen::OnCreationError(
    SupervisedUserCreationController::ErrorCode code) {
  LOG(ERROR) << "Supervised user creation failure, code: " << code;

  base::string16 title;
  base::string16 message;
  base::string16 button;
  // TODO(antrim) : find out which errors do we really have.
  // We might reuse some error messages from ordinary user flow.
  switch (code) {
    case SupervisedUserCreationController::CRYPTOHOME_NO_MOUNT:
    case SupervisedUserCreationController::CRYPTOHOME_FAILED_MOUNT:
    case SupervisedUserCreationController::CRYPTOHOME_FAILED_TPM:
      ::login::GetSecureModuleUsed(base::BindOnce(
          &SupervisedUserCreationScreen::UpdateSecureModuleMessages,
          weak_factory_.GetWeakPtr()));
      return;
    case SupervisedUserCreationController::CLOUD_SERVER_ERROR:
    case SupervisedUserCreationController::TOKEN_WRITE_FAILED:
      title = l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_GENERIC_ERROR_TITLE);
      message =
          l10n_util::GetStringUTF16(IDS_CREATE_SUPERVISED_USER_GENERIC_ERROR);
      button = l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_GENERIC_ERROR_BUTTON);
      break;
    case SupervisedUserCreationController::NO_ERROR:
      NOTREACHED();
  }
  if (view_)
    view_->ShowErrorPage(title, message, button);
}

void SupervisedUserCreationScreen::OnCreationTimeout() {
  if (view_) {
    view_->ShowStatusMessage(
        false /* error */,
        l10n_util::GetStringUTF16(
            IDS_CREATE_SUPERVISED_USER_CREATION_CREATION_TIMEOUT_MESSAGE));
  }
}

void SupervisedUserCreationScreen::OnLongCreationWarning() {
  if (view_) {
    view_->ShowStatusMessage(
        true /* progress */,
        l10n_util::GetStringUTF16(
            IDS_PROFILES_CREATE_SUPERVISED_JUST_SIGNED_IN));
  }
}

bool SupervisedUserCreationScreen::FindUserByDisplayName(
    const base::string16& display_name,
    std::string* out_id) const {
  return false;
}

// TODO(antrim) : this is an explicit code duplications with UserImageScreen.
// It should be removed by issue 251179.

void SupervisedUserCreationScreen::ApplyPicture() {
  std::string user_id = controller_->GetSupervisedUserId();
  UserImageManager* image_manager =
      ChromeUserManager::Get()->GetUserImageManager(
          AccountId::FromUserEmail(user_id));
  switch (selected_image_) {
    case user_manager::User::USER_IMAGE_EXTERNAL:
      // Photo decoding may not have been finished yet.
      if (user_photo_.isNull()) {
        apply_photo_after_decoding_ = true;
        return;
      }
      image_manager->SaveUserImage(user_manager::UserImage::CreateAndEncode(
          user_photo_, user_manager::UserImage::FORMAT_JPEG));
      break;
    case user_manager::User::USER_IMAGE_PROFILE:
      NOTREACHED() << "Supervised users have no profile pictures";
      break;
    default:
      DCHECK(default_user_image::IsValidIndex(selected_image_));
      image_manager->SaveUserDefaultImageIndex(selected_image_);
      break;
  }
  // Proceed to tutorial.
  view_->ShowTutorialPage();
}

void SupervisedUserCreationScreen::OnCreationSuccess() {
  ApplyPicture();
}

void SupervisedUserCreationScreen::OnCameraPresenceCheckDone(
    bool is_camera_present) {
  if (view_)
    view_->SetCameraPresent(is_camera_present);
}

void SupervisedUserCreationScreen::UpdateSecureModuleMessages(
    ::login::SecureModuleUsed secure_module_used) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::string16 title;
  base::string16 message;
  base::string16 button;
  switch (secure_module_used) {
    case ::login::SecureModuleUsed::TPM:
      title =
          l10n_util::GetStringUTF16(IDS_CREATE_SUPERVISED_USER_TPM_ERROR_TITLE);
      message = l10n_util::GetStringUTF16(IDS_CREATE_SUPERVISED_USER_TPM_ERROR);
      button = l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_TPM_ERROR_BUTTON);
      break;
    case ::login::SecureModuleUsed::CR50:
      title = l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_SECURE_MODULE_ERROR_TITLE);
      message = l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_SECURE_MODULE_ERROR);
      button = l10n_util::GetStringUTF16(
          IDS_CREATE_SUPERVISED_USER_SECURE_MODULE_ERROR_BUTTON);
      break;
    default:
      NOTREACHED();
      break;
  }
  if (view_)
    view_->ShowErrorPage(title, message, button);
}

void SupervisedUserCreationScreen::OnPhotoTaken(const std::string& raw_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  user_photo_ = gfx::ImageSkia();
  ImageDecoder::Cancel(this);
  ImageDecoder::Start(this, raw_data);
}

void SupervisedUserCreationScreen::OnImageDecoded(
    const SkBitmap& decoded_image) {
  user_photo_ = gfx::ImageSkia::CreateFrom1xBitmap(decoded_image);
  if (apply_photo_after_decoding_)
    ApplyPicture();
}

void SupervisedUserCreationScreen::OnDecodeImageFailed() {
  NOTREACHED() << "Failed to decode PNG image from WebUI";
}

void SupervisedUserCreationScreen::OnImageSelected(
    const std::string& image_type,
    const std::string& image_url) {
  if (image_type == "default") {
    int user_image_index = user_manager::User::USER_IMAGE_INVALID;
    if (image_url.empty() ||
        !default_user_image::IsDefaultImageUrl(image_url, &user_image_index)) {
      LOG(ERROR) << "Unexpected default image url: " << image_url;
      return;
    }
    selected_image_ = user_image_index;
  } else if (image_type == "camera" || image_type == "old") {
    selected_image_ = user_manager::User::USER_IMAGE_EXTERNAL;
  } else {
    NOTREACHED() << "Unexpected image type: " << image_type;
  }
}

void SupervisedUserCreationScreen::OnImageAccepted() {}

}  // namespace chromeos
