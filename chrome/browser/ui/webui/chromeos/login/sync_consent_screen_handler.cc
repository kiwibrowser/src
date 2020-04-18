// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/sync_consent_screen_handler.h"

#include "chrome/browser/chromeos/login/screens/sync_consent_screen.h"
#include "chrome/grit/generated_resources.h"
#include "components/login/localized_values_builder.h"

namespace {

const char kJsScreenPath[] = "login.SyncConsentScreen";

}  // namespace

namespace chromeos {

SyncConsentScreenHandler::SyncConsentScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
}

SyncConsentScreenHandler::~SyncConsentScreenHandler() {}

void SyncConsentScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("syncConsentScreenTitle", IDS_LOGIN_SYNC_CONSENT_SCREEN_TITLE);
  builder->Add("syncConsentScreenChromeSyncName",
               IDS_LOGIN_SYNC_CONSENT_SCREEN_CHROME_SYNC_NAME);
  builder->Add("syncConsentScreenChromeSyncDescription",
               IDS_LOGIN_SYNC_CONSENT_SCREEN_CHROME_SYNC_DESCRIPTION);
  builder->Add("syncConsentScreenPersonalizeGoogleServicesName",
               IDS_LOGIN_SYNC_CONSENT_SCREEN_PERSONALIZE_GOOGLE_SERVICES_NAME);
  builder->Add(
      "syncConsentScreenPersonalizeGoogleServicesDescription",
      IDS_LOGIN_SYNC_CONSENT_SCREEN_PERSONALIZE_GOOGLE_SERVICES_DESCRIPTION);
  builder->Add("syncConsentReviewSyncOptionsText",
               IDS_LOGIN_SYNC_CONSENT_SCREEN_REVIEW_SYNC_OPTIONS_LATER);

  builder->Add("syncConsentNewScreenTitle",
               IDS_LOGIN_SYNC_CONSENT_GET_GOOGLE_SMARTS);
  builder->Add("syncConsentNewBookmarksDesc",
               IDS_LOGIN_SYNC_CONSENT_YOUR_BOOKMARKS_ON_ALL_DEVICES);
  builder->Add("syncConsentNewServicesDesc",
               IDS_LOGIN_SYNC_CONSENT_PERSONALIZED_GOOGLE_SERVICES);
  builder->Add("syncConsentNewImproveChrome",
               IDS_LOGIN_SYNC_CONSENT_IMPROVE_CHROME);
  builder->Add("syncConsentNewGoogleMayUse",
               IDS_LOGIN_SYNC_CONSENT_GOOGLE_MAY_USE);
  builder->Add("syncConsentNewMoreOptions",
               IDS_LOGIN_SYNC_CONSENT_MORE_OPTIONS);
  builder->Add("syncConsentNewYesIAmIn", IDS_LOGIN_SYNC_CONSENT_YES_I_AM_IN);
  builder->Add("syncConsentNewSyncOptions",
               IDS_LOGIN_SYNC_CONSENT_SYNC_OPTIONS);
  builder->Add("syncConsentNewSyncOptionsSubtitle",
               IDS_LOGIN_SYNC_CONSENT_SYNC_OPTIONS_SUBTITLE);
  builder->Add("syncConsentNewChooseOption",
               IDS_LOGIN_SYNC_CONSENT_CHOOSE_OPTION);
  builder->Add("syncConsentNewOptionReview",
               IDS_LOGIN_SYNC_CONSENT_OPTION_REVIEW);
  builder->Add("syncConsentNewOptionReviewDsc",
               IDS_LOGIN_SYNC_CONSENT_OPTION_REVIEW_DSC);
  builder->Add("syncConsentNewOptionJustSync",
               IDS_LOGIN_SYNC_CONSENT_OPTION_JUST_SYNC);
  builder->Add("syncConsentNewOptionJustSyncDsc",
               IDS_LOGIN_SYNC_CONSENT_OPTION_JUST_SYNC_DSC);
  builder->Add("syncConsentNewOptionSyncAndPersonalization",
               IDS_LOGIN_SYNC_CONSENT_OPTION_SYNC_AND_PERSONALIZATION);
  builder->Add("syncConsentNewOptionSyncAndPersonalizationDsc",
               IDS_LOGIN_SYNC_CONSENT_OPTION_SYNC_AND_PERSONALIZATION_DSC);

  builder->Add("syncConsentAcceptAndContinue",
               IDS_LOGIN_SYNC_CONSENT_SCREEN_ACCEPT_AND_CONTINUE);
}

void SyncConsentScreenHandler::Bind(SyncConsentScreen* screen) {
  screen_ = screen;
  BaseScreenHandler::SetBaseScreen(screen);
}

void SyncConsentScreenHandler::Show() {
  ShowScreen(kScreenId);
}

void SyncConsentScreenHandler::Hide() {}

void SyncConsentScreenHandler::SetThrobberVisible(bool visible) {
  CallJS("setThrobberVisible", visible);
}

void SyncConsentScreenHandler::Initialize() {}

void SyncConsentScreenHandler::GetAdditionalParameters(
    base::DictionaryValue* parameters) {
  parameters->Set("syncConsentMakeBetter",
                  std::make_unique<base::Value>(false));
  BaseScreenHandler::GetAdditionalParameters(parameters);
}

}  // namespace chromeos
