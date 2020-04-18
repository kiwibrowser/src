// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/arc_kiosk_splash_screen_handler.h"

#include <memory>

#include "base/values.h"
#include "chrome/grit/chrome_unscaled_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/login/localized_values_builder.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/gfx/image/image_skia.h"

namespace {

constexpr char kJsScreenPath[] = "login.ArcKioskSplashScreen";
}

namespace chromeos {

ArcKioskSplashScreenHandler::ArcKioskSplashScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
}

ArcKioskSplashScreenHandler::~ArcKioskSplashScreenHandler() {
  if (delegate_)
    delegate_->OnDeletingSplashScreenView();
}

void ArcKioskSplashScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("arcKioskStartMessage", IDS_APP_START_APP_WAIT_MESSAGE);

  const base::string16 product_os_name =
      l10n_util::GetStringUTF16(IDS_SHORT_PRODUCT_OS_NAME);
  builder->Add("arcKioskShortcutInfo",
               l10n_util::GetStringFUTF16(IDS_APP_START_BAILOUT_SHORTCUT_FORMAT,
                                          product_os_name));
  builder->Add("arcKioskProductName", product_os_name);
}

void ArcKioskSplashScreenHandler::Initialize() {
  if (!show_on_init_)
    return;
  show_on_init_ = false;
  Show();
}

void ArcKioskSplashScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }

  base::DictionaryValue data;
  // |data| will take ownership of |app_info|.
  std::unique_ptr<base::DictionaryValue> app_info =
      std::make_unique<base::DictionaryValue>();
  PopulateAppInfo(app_info.get());
  data.Set("appInfo", std::move(app_info));
  ShowScreenWithData(kScreenId, &data);
}

void ArcKioskSplashScreenHandler::RegisterMessages() {
  AddCallback("cancelArcKioskLaunch",
              &ArcKioskSplashScreenHandler::HandleCancelArcKioskLaunch);
}

void ArcKioskSplashScreenHandler::UpdateArcKioskState(ArcKioskState state) {
  if (!page_is_ready())
    return;
  SetLaunchText(l10n_util::GetStringUTF8(GetProgressMessageFromState(state)));
}

void ArcKioskSplashScreenHandler::SetDelegate(
    ArcKioskSplashScreenHandler::Delegate* delegate) {
  delegate_ = delegate;
}

void ArcKioskSplashScreenHandler::PopulateAppInfo(
    base::DictionaryValue* out_info) {
  out_info->SetString("name", l10n_util::GetStringUTF8(IDS_SHORT_PRODUCT_NAME));
  out_info->SetString(
      "iconURL",
      webui::GetBitmapDataUrl(*ui::ResourceBundle::GetSharedInstance()
                                   .GetImageSkiaNamed(IDR_PRODUCT_LOGO_128)
                                   ->bitmap()));
}

void ArcKioskSplashScreenHandler::SetLaunchText(const std::string& text) {
  CallJS("updateArcKioskMessage", text);
}

int ArcKioskSplashScreenHandler::GetProgressMessageFromState(
    ArcKioskState state) {
  switch (state) {
    case ArcKioskState::STARTING_SESSION:
      return IDS_SYNC_SETUP_SPINNER_TITLE;
    case ArcKioskState::WAITING_APP_LAUNCH:
      return IDS_APP_START_APP_WAIT_MESSAGE;
    case ArcKioskState::WAITING_APP_WINDOW:
      return IDS_APP_START_WAIT_FOR_APP_WINDOW_MESSAGE;
    default:
      NOTREACHED();
      break;
  }
  return IDS_SYNC_SETUP_SPINNER_TITLE;
}

void ArcKioskSplashScreenHandler::HandleCancelArcKioskLaunch() {
  if (!delegate_) {
    LOG(WARNING) << "No delegate set to handle cancel app launch";
    return;
  }
  delegate_->OnCancelArcKioskLaunch();
}

}  // namespace chromeos
