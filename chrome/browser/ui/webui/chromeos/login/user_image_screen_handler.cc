// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/user_image_screen_handler.h"

#include <utility>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/values.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/user_image_screen.h"
#include "chrome/browser/chromeos/login/ui/login_display_webui.h"
#include "chrome/browser/chromeos/login/users/default_user_image/default_user_images.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/audio/chromeos_sounds.h"
#include "components/login/localized_values_builder.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_manager/user.h"
#include "media/audio/sounds/sounds_manager.h"
#include "net/base/data_url.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace {

const char kJsScreenPath[] = "login.UserImageScreen";

}  // namespace

namespace chromeos {

UserImageScreenHandler::UserImageScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  media::SoundsManager* manager = media::SoundsManager::Get();
  manager->Initialize(SOUND_OBJECT_DELETE,
                      bundle.GetRawDataResource(IDR_SOUND_OBJECT_DELETE_WAV));
  manager->Initialize(SOUND_CAMERA_SNAP,
                      bundle.GetRawDataResource(IDR_SOUND_CAMERA_SNAP_WAV));
}

UserImageScreenHandler::~UserImageScreenHandler() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void UserImageScreenHandler::Initialize() {
  if (show_on_init_) {
    Show();
    show_on_init_ = false;
  }
}

void UserImageScreenHandler::Bind(UserImageScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen_);
}

void UserImageScreenHandler::Unbind() {
  screen_ = nullptr;
  BaseScreenHandler::SetBaseScreen(nullptr);
}

void UserImageScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }
  screen_show_time_ = base::Time::Now();
  ShowScreen(kScreenId);

  // When shown, query camera presence.
  if (screen_ && is_ready_)
    screen_->OnScreenReady();
}

void UserImageScreenHandler::Hide() {
}

void UserImageScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("userImageScreenTitle", IDS_USER_IMAGE_SCREEN_TITLE);
  builder->Add("userImageScreenDescription",
               IDS_USER_IMAGE_SCREEN_DESCRIPTION);
  builder->Add("takePhoto", IDS_OPTIONS_CHANGE_PICTURE_TAKE_PHOTO);
  builder->Add("captureVideo", IDS_OPTIONS_CHANGE_PICTURE_CAPTURE_VIDEO);
  builder->Add("discardPhoto", IDS_OPTIONS_CHANGE_PICTURE_DISCARD_PHOTO);
  builder->Add("switchModeToCamera",
               IDS_OPTIONS_CHANGE_PICTURE_SWITCH_MODE_TO_CAMERA);
  builder->Add("switchModeToVideo",
               IDS_OPTIONS_CHANGE_PICTURE_SWITCH_MODE_TO_VIDEO);
  builder->Add("profilePhoto", IDS_IMAGE_SCREEN_PROFILE_PHOTO);
  builder->Add("profilePhotoLoading",
               IDS_IMAGE_SCREEN_PROFILE_LOADING_PHOTO);
  builder->Add("okButtonText", IDS_OK);
  builder->Add("photoFromCamera", IDS_OPTIONS_CHANGE_PICTURE_PHOTO_FROM_CAMERA);
  builder->Add("syncingPreferences", IDS_IMAGE_SCREEN_SYNCING_PREFERENCES);
}

void UserImageScreenHandler::RegisterMessages() {
  AddCallback("getImages", &UserImageScreenHandler::HandleGetImages);
  AddCallback("screenReady", &UserImageScreenHandler::HandleScreenReady);
  AddCallback("discardPhoto", &UserImageScreenHandler::HandleDiscardPhoto);
  AddCallback("photoTaken", &UserImageScreenHandler::HandlePhotoTaken);
  AddCallback("selectImage", &UserImageScreenHandler::HandleSelectImage);
  AddCallback("onUserImageAccepted",
              &UserImageScreenHandler::HandleImageAccepted);
  AddCallback("onUserImageScreenShown",
              &UserImageScreenHandler::HandleScreenShown);
}

// TODO(antrim) : It looks more like parameters for "Init" rather than callback.
void UserImageScreenHandler::HandleGetImages() {
  base::DictionaryValue result;
  result.SetInteger("first", default_user_image::GetFirstDefaultImage());
  std::unique_ptr<base::ListValue> default_images =
      default_user_image::GetAsDictionary(true /* all */);
  result.Set("images", std::move(default_images));
  CallJS("setDefaultImages", result);
}

void UserImageScreenHandler::HandleScreenReady() {
  is_ready_ = true;
  if (screen_)
    screen_->OnScreenReady();
}

void UserImageScreenHandler::HandlePhotoTaken(const std::string& image_url) {
  AccessibilityManager::Get()->PlayEarcon(
      SOUND_CAMERA_SNAP, PlaySoundOption::ONLY_IF_SPOKEN_FEEDBACK_ENABLED);

  std::string raw_data;
  base::StringPiece url(image_url);
  const char kDataUrlPrefix[] = "data:image/png;base64,";
  const size_t kDataUrlPrefixLength = arraysize(kDataUrlPrefix) - 1;
  if (!url.starts_with(kDataUrlPrefix) ||
      !base::Base64Decode(url.substr(kDataUrlPrefixLength), &raw_data)) {
    LOG(WARNING) << "Invalid image URL";
    return;
  }

  if (screen_)
    screen_->OnPhotoTaken(raw_data);
}

void UserImageScreenHandler::HandleDiscardPhoto() {
  AccessibilityManager::Get()->PlayEarcon(
      SOUND_OBJECT_DELETE, PlaySoundOption::ONLY_IF_SPOKEN_FEEDBACK_ENABLED);
}

void UserImageScreenHandler::HandleSelectImage(const std::string& image_type,
                                               const std::string& image_url,
                                               bool is_user_selection) {
  if (screen_)
    screen_->OnImageSelected(image_type, image_url, is_user_selection);
}

void UserImageScreenHandler::HandleImageAccepted() {
  if (screen_)
    screen_->OnImageAccepted();
}

void UserImageScreenHandler::HandleScreenShown() {
  DCHECK(!screen_show_time_.is_null());

  base::TimeDelta delta = base::Time::Now() - screen_show_time_;
  VLOG(1) << "Screen load time: " << delta.InSecondsF();
  UMA_HISTOGRAM_TIMES("UserImage.ScreenIsShownTime", delta);
}

void UserImageScreenHandler::HideCurtain() {
  CallJS("hideCurtain");
}

}  // namespace chromeos
