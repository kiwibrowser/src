// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/supervised_user_creation_screen_handler.h"

#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/user_selection_screen.h"
#include "chrome/browser/chromeos/login/supervised/supervised_user_creation_flow.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/supervised_user_manager.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/audio/chromeos_sounds.h"
#include "components/account_id/account_id.h"
#include "components/login/localized_values_builder.h"
#include "components/user_manager/user_manager.h"
#include "components/user_manager/user_type.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "media/audio/sounds/sounds_manager.h"
#include "net/base/data_url.h"
#include "net/base/escape.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

const char kJsScreenPath[] = "login.SupervisedUserCreationScreen";

namespace chromeos {

SupervisedUserCreationScreenHandler::SupervisedUserCreationScreenHandler()
    : BaseScreenHandler(OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW) {
  set_call_js_prefix(kJsScreenPath);
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  media::SoundsManager* manager = media::SoundsManager::Get();
  manager->Initialize(SOUND_OBJECT_DELETE,
                      bundle.GetRawDataResource(IDR_SOUND_OBJECT_DELETE_WAV));
  manager->Initialize(SOUND_CAMERA_SNAP,
                      bundle.GetRawDataResource(IDR_SOUND_CAMERA_SNAP_WAV));
}

SupervisedUserCreationScreenHandler::~SupervisedUserCreationScreenHandler() {
  if (delegate_) {
    delegate_->OnViewDestroyed(this);
  }
}

void SupervisedUserCreationScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add(
      "supervisedUserCreationFlowRetryButtonTitle",
      IDS_CREATE_SUPERVISED_USER_CREATION_ERROR_RETRY_BUTTON_TITLE);
  builder->Add(
      "supervisedUserCreationFlowCancelButtonTitle",
      IDS_CREATE_SUPERVISED_USER_CREATION_ERROR_CANCEL_BUTTON_TITLE);
  builder->Add(
      "supervisedUserCreationFlowGotitButtonTitle",
       IDS_CREATE_SUPERVISED_USER_CREATION_GOT_IT_BUTTON_TITLE);

  builder->Add("createSupervisedUserIntroTextTitle",
               IDS_CREATE_SUPERVISED_INTRO_TEXT_TITLE);
  builder->Add("createSupervisedUserIntroAlternateText",
               IDS_CREATE_SUPERVISED_INTRO_ALTERNATE_TEXT);
  builder->Add("createSupervisedUserIntroText1",
               IDS_CREATE_SUPERVISED_INTRO_TEXT_1);
  builder->Add("createSupervisedUserIntroManagerItem1",
               IDS_CREATE_SUPERVISED_INTRO_MANAGER_ITEM_1);
  builder->Add("createSupervisedUserIntroManagerItem2",
               IDS_CREATE_SUPERVISED_INTRO_MANAGER_ITEM_2);
  builder->Add("createSupervisedUserIntroManagerItem3",
               IDS_CREATE_SUPERVISED_INTRO_MANAGER_ITEM_3);
  builder->Add("createSupervisedUserIntroText2",
               IDS_CREATE_SUPERVISED_INTRO_TEXT_2);
  builder->AddF("createSupervisedUserIntroText3",
               IDS_CREATE_SUPERVISED_INTRO_TEXT_3,
               base::UTF8ToUTF16(
                   chrome::kLegacySupervisedUserManagementDisplayURL));

  builder->Add("createSupervisedUserPickManagerTitle",
               IDS_CREATE_SUPERVISED_USER_CREATE_PICK_MANAGER_TITLE);
  builder->AddF("createSupervisedUserPickManagerTitleExplanation",
               IDS_CREATE_SUPERVISED_USER_CREATE_PICK_MANAGER_EXPLANATION,
               base::UTF8ToUTF16(
                   chrome::kLegacySupervisedUserManagementDisplayURL));
  builder->Add("createSupervisedUserManagerPasswordHint",
               IDS_CREATE_SUPERVISED_USER_CREATE_MANAGER_PASSWORD_HINT);
  builder->Add("createSupervisedUserWrongManagerPasswordText",
               IDS_CREATE_SUPERVISED_USER_MANAGER_PASSWORD_ERROR);

  builder->Add("createSupervisedUserNameTitle",
               IDS_CREATE_SUPERVISED_USER_CREATE_ACCOUNT_NAME_TITLE);
  builder->Add("createSupervisedUserNameAccessibleTitle",
               IDS_CREATE_SUPERVISED_USER_SETUP_ACCESSIBLE_TITLE);
  builder->Add("createSupervisedUserNameExplanation",
               IDS_CREATE_SUPERVISED_USER_CREATE_ACCOUNT_NAME_EXPLANATION);
  builder->Add("createSupervisedUserNameHint",
               IDS_CREATE_SUPERVISED_USER_CREATE_ACCOUNT_NAME_HINT);
  builder->Add("createSupervisedUserPasswordTitle",
               IDS_CREATE_SUPERVISED_USER_CREATE_PASSWORD_TITLE);
  builder->Add("createSupervisedUserPasswordExplanation",
               IDS_CREATE_SUPERVISED_USER_CREATE_PASSWORD_EXPLANATION);
  builder->Add("createSupervisedUserPasswordHint",
               IDS_CREATE_SUPERVISED_USER_CREATE_PASSWORD_HINT);
  builder->Add("createSupervisedUserPasswordConfirmHint",
               IDS_CREATE_SUPERVISED_USER_CREATE_PASSWORD_CONFIRM_HINT);
  builder->Add("supervisedUserCreationFlowProceedButtonTitle",
               IDS_CREATE_SUPERVISED_USER_CREATE_CONTINUE_BUTTON_TEXT);
  builder->Add("supervisedUserCreationFlowStartButtonTitle",
               IDS_CREATE_SUPERVISED_USER_CREATE_START_BUTTON_TEXT);
  builder->Add("supervisedUserCreationFlowPrevButtonTitle",
               IDS_CREATE_SUPERVISED_USER_CREATE_PREVIOUS_BUTTON_TEXT);
  builder->Add("supervisedUserCreationFlowNextButtonTitle",
               IDS_CREATE_SUPERVISED_USER_CREATE_NEXT_BUTTON_TEXT);
  builder->Add("supervisedUserCreationFlowHandleErrorButtonTitle",
               IDS_CREATE_SUPERVISED_USER_CREATE_HANDLE_ERROR_BUTTON_TEXT);
  builder->Add("createSupervisedUserPasswordMismatchError",
               IDS_CREATE_SUPERVISED_USER_CREATE_PASSWORD_MISMATCH_ERROR);

  builder->Add("createSupervisedUserCreatedText1",
               IDS_CREATE_SUPERVISED_USER_CREATED_1_TEXT_1);
  builder->Add("createSupervisedUserCreatedText2",
               IDS_CREATE_SUPERVISED_USER_CREATED_1_TEXT_2);
  builder->Add("createSupervisedUserCreatedText3",
               IDS_CREATE_SUPERVISED_USER_CREATED_1_TEXT_3);

  builder->Add("importExistingSupervisedUserTitle",
               IDS_IMPORT_EXISTING_LEGACY_SUPERVISED_USER_TITLE);
  builder->Add("importSupervisedUserLink",
               IDS_IMPORT_EXISTING_LEGACY_SUPERVISED_USER_TITLE);
  builder->Add("importExistingSupervisedUserText",
               IDS_IMPORT_EXISTING_LEGACY_SUPERVISED_USER_TEXT);
  builder->Add("supervisedUserCreationFlowImportButtonTitle",
               IDS_IMPORT_EXISTING_LEGACY_SUPERVISED_USER_OK);
  builder->Add("createSupervisedUserLink",
               IDS_CREATE_NEW_LEGACY_SUPERVISED_USER_LINK);
  builder->Add("importBubbleText",
               IDS_SUPERVISED_USER_IMPORT_BUBBLE_TEXT);
  builder->Add("importUserExists",
               IDS_SUPERVISED_USER_IMPORT_USER_EXIST);
  builder->Add("importUsernameExists",
               IDS_SUPERVISED_USER_IMPORT_USERNAME_EXIST);

  builder->Add("managementURL",
               chrome::kLegacySupervisedUserManagementDisplayURL);

  // TODO(antrim) : this is an explicit code duplications with UserImageScreen.
  // It should be removed by issue 251179.
  builder->Add("takePhoto", IDS_OPTIONS_CHANGE_PICTURE_TAKE_PHOTO);
  builder->Add("discardPhoto", IDS_OPTIONS_CHANGE_PICTURE_DISCARD_PHOTO);
  builder->Add("photoCaptureAccessibleText",
               IDS_OPTIONS_PHOTO_CAPTURE_ACCESSIBLE_TEXT);
  builder->Add("photoDiscardAccessibleText",
               IDS_OPTIONS_PHOTO_DISCARD_ACCESSIBLE_TEXT);
}

void SupervisedUserCreationScreenHandler::Initialize() {}

void SupervisedUserCreationScreenHandler::RegisterMessages() {
  AddCallback("finishLocalSupervisedUserCreation",
              &SupervisedUserCreationScreenHandler::
                  HandleFinishLocalSupervisedUserCreation);
  AddCallback("abortLocalSupervisedUserCreation",
              &SupervisedUserCreationScreenHandler::
                  HandleAbortLocalSupervisedUserCreation);
  AddCallback("hideLocalSupervisedUserCreation",
              &SupervisedUserCreationScreenHandler::
                  HandleHideLocalSupervisedUserCreation);
  AddCallback("checkSupervisedUserName",
              &SupervisedUserCreationScreenHandler::
                  HandleCheckSupervisedUserName);
  AddCallback("authenticateManagerInSupervisedUserCreationFlow",
              &SupervisedUserCreationScreenHandler::
                  HandleAuthenticateManager);
  AddCallback("specifySupervisedUserCreationFlowUserData",
              &SupervisedUserCreationScreenHandler::
                  HandleCreateSupervisedUser);
  AddCallback("managerSelectedOnSupervisedUserCreationFlow",
              &SupervisedUserCreationScreenHandler::
                  HandleManagerSelected);
  AddCallback("userSelectedForImportInSupervisedUserCreationFlow",
              &SupervisedUserCreationScreenHandler::
                  HandleImportUserSelected);
  AddCallback("importSupervisedUser",
              &SupervisedUserCreationScreenHandler::
                  HandleImportSupervisedUser);
  AddCallback("importSupervisedUserWithPassword",
              &SupervisedUserCreationScreenHandler::
                  HandleImportSupervisedUserWithPassword);


  // TODO(antrim) : this is an explicit code duplications with UserImageScreen.
  // It should be removed by issue 251179.
  AddCallback("supervisedUserGetImages",
              &SupervisedUserCreationScreenHandler::HandleGetImages);

  AddCallback("supervisedUserPhotoTaken",
              &SupervisedUserCreationScreenHandler::HandlePhotoTaken);
  AddCallback("supervisedUserTakePhoto",
              &SupervisedUserCreationScreenHandler::HandleTakePhoto);
  AddCallback("supervisedUserDiscardPhoto",
              &SupervisedUserCreationScreenHandler::HandleDiscardPhoto);
  AddCallback("supervisedUserSelectImage",
              &SupervisedUserCreationScreenHandler::HandleSelectImage);
  AddCallback("currentSupervisedUserPage",
              &SupervisedUserCreationScreenHandler::
                  HandleCurrentSupervisedUserPage);
}

void SupervisedUserCreationScreenHandler::Show() {
  std::unique_ptr<base::DictionaryValue> data(new base::DictionaryValue());
  std::unique_ptr<base::ListValue> users_list(new base::ListValue());
  const user_manager::UserList& users =
      ChromeUserManager::Get()->GetUsersAllowedForSupervisedUsersCreation();
  std::string owner;
  chromeos::CrosSettings::Get()->GetString(chromeos::kDeviceOwner, &owner);

  for (user_manager::UserList::const_iterator it = users.begin();
       it != users.end();
       ++it) {
    bool is_owner = ((*it)->GetAccountId().GetUserEmail() == owner);
    auto user_dict = std::make_unique<base::DictionaryValue>();
    UserSelectionScreen::FillUserDictionary(
        *it, is_owner, false, /* is_signin_to_add */
        proximity_auth::mojom::AuthType::OFFLINE_PASSWORD,
        NULL, /* public_session_recommended_locales */
        user_dict.get());
    users_list->Append(std::move(user_dict));
  }
  data->Set("managers", std::move(users_list));
  ShowScreenWithData(OobeScreen::SCREEN_CREATE_SUPERVISED_USER_FLOW,
                     data.get());

  if (!delegate_)
    return;
}

void SupervisedUserCreationScreenHandler::Hide() {
}

void SupervisedUserCreationScreenHandler::ShowIntroPage() {
  CallJS("showIntroPage");
}

void SupervisedUserCreationScreenHandler::ShowManagerPasswordError() {
  CallJS("showManagerPasswordError");
}

void SupervisedUserCreationScreenHandler::ShowStatusMessage(
    bool is_progress,
    const base::string16& message) {
  if (is_progress)
    CallJS("showProgress", message);
  else
    CallJS("showStatusError", message);
}

void SupervisedUserCreationScreenHandler::ShowUsernamePage() {
  CallJS("showUsernamePage");
}

void SupervisedUserCreationScreenHandler::ShowTutorialPage() {
  CallJS("showTutorialPage");
}

void SupervisedUserCreationScreenHandler::ShowErrorPage(
    const base::string16& title,
    const base::string16& message,
    const base::string16& button_text) {
  CallJS("showErrorPage", title, message, button_text);
}

void SupervisedUserCreationScreenHandler::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

void SupervisedUserCreationScreenHandler::
    HandleFinishLocalSupervisedUserCreation() {
  delegate_->FinishFlow();
}

void SupervisedUserCreationScreenHandler::
    HandleAbortLocalSupervisedUserCreation() {
  delegate_->AbortFlow();
}

void SupervisedUserCreationScreenHandler::
    HandleHideLocalSupervisedUserCreation() {
  delegate_->HideFlow();
}

void SupervisedUserCreationScreenHandler::HandleManagerSelected(
    const AccountId& manager_id) {
  if (!delegate_)
    return;
  WallpaperControllerClient::Get()->ShowUserWallpaper(manager_id);
}

void SupervisedUserCreationScreenHandler::HandleImportUserSelected(
    const AccountId& account_id) {
  if (!delegate_)
    return;
}

void SupervisedUserCreationScreenHandler::HandleCheckSupervisedUserName(
    const base::string16& name) {
  std::string user_id;
  if (NULL !=
      ChromeUserManager::Get()->GetSupervisedUserManager()->FindByDisplayName(
          base::CollapseWhitespace(name, true))) {
    CallJS("supervisedUserNameError", name,
           l10n_util::GetStringUTF16(
               IDS_CREATE_SUPERVISED_USER_CREATE_USERNAME_ALREADY_EXISTS));
  } else if (net::EscapeForHTML(name) != name) {
    CallJS("supervisedUserNameError", name,
           l10n_util::GetStringUTF16(
               IDS_CREATE_SUPERVISED_USER_CREATE_ILLEGAL_USERNAME));
  } else if (delegate_ && delegate_->FindUserByDisplayName(
                 base::CollapseWhitespace(name, true), &user_id)) {
    CallJS("supervisedUserSuggestImport", name, user_id);
  } else {
    CallJS("supervisedUserNameOk", name);
  }
}

void SupervisedUserCreationScreenHandler::HandleCreateSupervisedUser(
    const base::string16& new_raw_user_name,
    const std::string& new_user_password) {
  if (!delegate_)
    return;
  const base::string16 new_user_name =
      base::CollapseWhitespace(new_raw_user_name, true);
  if (NULL !=
      ChromeUserManager::Get()->GetSupervisedUserManager()->FindByDisplayName(
          new_user_name)) {
    CallJS("supervisedUserNameError", new_user_name,
           l10n_util::GetStringFUTF16(
               IDS_CREATE_SUPERVISED_USER_CREATE_USERNAME_ALREADY_EXISTS,
               new_user_name));
    return;
  }
  if (net::EscapeForHTML(new_user_name) != new_user_name) {
    CallJS("supervisedUserNameError", new_user_name,
           l10n_util::GetStringUTF16(
               IDS_CREATE_SUPERVISED_USER_CREATE_ILLEGAL_USERNAME));
    return;
  }

  if (new_user_password.length() == 0) {
    CallJS("showPasswordError",
           l10n_util::GetStringUTF16(
               IDS_CREATE_SUPERVISED_USER_CREATE_PASSWORD_TOO_SHORT));
    return;
  }

  ShowStatusMessage(true /* progress */, l10n_util::GetStringUTF16(
      IDS_CREATE_SUPERVISED_USER_CREATION_CREATION_PROGRESS_MESSAGE));

  delegate_->CreateSupervisedUser(new_user_name, new_user_password);
}

void SupervisedUserCreationScreenHandler::HandleImportSupervisedUser(
    const AccountId& account_id) {
  if (!delegate_)
    return;

  ShowStatusMessage(true /* progress */, l10n_util::GetStringUTF16(
      IDS_CREATE_SUPERVISED_USER_CREATION_CREATION_PROGRESS_MESSAGE));

  delegate_->ImportSupervisedUser(account_id.GetUserEmail());
}

void SupervisedUserCreationScreenHandler::
    HandleImportSupervisedUserWithPassword(const AccountId& account_id,
                                           const std::string& password) {
  if (!delegate_)
    return;

  ShowStatusMessage(true /* progress */, l10n_util::GetStringUTF16(
      IDS_CREATE_SUPERVISED_USER_CREATION_CREATION_PROGRESS_MESSAGE));

  delegate_->ImportSupervisedUserWithPassword(account_id.GetUserEmail(),
                                              password);
}

void SupervisedUserCreationScreenHandler::HandleAuthenticateManager(
    const AccountId& manager_raw_account_id,
    const std::string& manager_password) {
  const AccountId manager_account_id = AccountId::FromUserEmailGaiaId(
      gaia::SanitizeEmail(manager_raw_account_id.GetUserEmail()),
      manager_raw_account_id.GetGaiaId());
  delegate_->AuthenticateManager(manager_account_id, manager_password);
}

// TODO(antrim) : this is an explicit code duplications with UserImageScreen.
// It should be removed by issue 251179.
void SupervisedUserCreationScreenHandler::HandleGetImages() {
  base::DictionaryValue result;
  result.SetInteger("first", default_user_image::GetFirstDefaultImage());
  std::unique_ptr<base::ListValue> default_images =
      default_user_image::GetAsDictionary(true /* all */);
  result.Set("images", std::move(default_images));
  CallJS("setDefaultImages", result);
}

void SupervisedUserCreationScreenHandler::HandlePhotoTaken
    (const std::string& image_url) {
  std::string mime_type, charset, raw_data;
  if (!net::DataURL::Parse(GURL(image_url), &mime_type, &charset, &raw_data))
    NOTREACHED();
  DCHECK_EQ("image/png", mime_type);

  if (delegate_)
    delegate_->OnPhotoTaken(raw_data);
}

void SupervisedUserCreationScreenHandler::HandleTakePhoto() {
  AccessibilityManager::Get()->PlayEarcon(
      SOUND_CAMERA_SNAP, PlaySoundOption::ONLY_IF_SPOKEN_FEEDBACK_ENABLED);
}

void SupervisedUserCreationScreenHandler::HandleDiscardPhoto() {
  AccessibilityManager::Get()->PlayEarcon(
      SOUND_OBJECT_DELETE, PlaySoundOption::ONLY_IF_SPOKEN_FEEDBACK_ENABLED);
}

void SupervisedUserCreationScreenHandler::HandleSelectImage(
    const std::string& image_type,
    const std::string& image_url) {
  if (delegate_)
    delegate_->OnImageSelected(image_type, image_url);
}

void SupervisedUserCreationScreenHandler::HandleCurrentSupervisedUserPage(
    const std::string& page) {
  if (delegate_)
    delegate_->OnPageSelected(page);
}

void SupervisedUserCreationScreenHandler::ShowPage(
    const std::string& page) {
  CallJS("showPage", page);
}

void SupervisedUserCreationScreenHandler::SetCameraPresent(bool present) {
  CallJS("setCameraPresent", present);
}

void SupervisedUserCreationScreenHandler::ShowExistingSupervisedUsers(
    const base::ListValue* users) {
  CallJS("setExistingSupervisedUsers", *users);
}

}  // namespace chromeos
