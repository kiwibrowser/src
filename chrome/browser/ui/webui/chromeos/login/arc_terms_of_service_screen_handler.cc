// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/arc_terms_of_service_screen_handler.h"

#include "base/command_line.h"
#include "base/i18n/timezone.h"
#include "chrome/browser/chromeos/arc/arc_support_host.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/optin/arc_optin_preference_handler.h"
#include "chrome/browser/chromeos/login/screens/arc_terms_of_service_screen_view_observer.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/consent_auditor/consent_auditor_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "components/arc/arc_prefs.h"
#include "components/consent_auditor/consent_auditor.h"
#include "components/login/localized_values_builder.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

const char kJsScreenPath[] = "login.ArcTermsOfServiceScreen";

}  // namespace

namespace chromeos {

ArcTermsOfServiceScreenHandler::ArcTermsOfServiceScreenHandler()
    : BaseScreenHandler(kScreenId) {
  set_call_js_prefix(kJsScreenPath);
}

ArcTermsOfServiceScreenHandler::~ArcTermsOfServiceScreenHandler() {
  OobeUI* oobe_ui = GetOobeUI();
  if (oobe_ui)
    oobe_ui->RemoveObserver(this);
  chromeos::NetworkHandler::Get()->network_state_handler()->RemoveObserver(
      this, FROM_HERE);
  system::TimezoneSettings::GetInstance()->RemoveObserver(this);
  for (auto& observer : observer_list_)
    observer.OnViewDestroyed(this);
}

void ArcTermsOfServiceScreenHandler::RegisterMessages() {
  AddCallback("arcTermsOfServiceSkip",
              &ArcTermsOfServiceScreenHandler::HandleSkip);
  AddCallback("arcTermsOfServiceAccept",
              &ArcTermsOfServiceScreenHandler::HandleAccept);
}

void ArcTermsOfServiceScreenHandler::MaybeLoadPlayStoreToS(
    bool ignore_network_state) {
  const chromeos::NetworkState* default_network =
      chromeos::NetworkHandler::Get()
          ->network_state_handler()
          ->DefaultNetwork();
  if (!ignore_network_state && !default_network)
    return;
  const std::string country_code = base::CountryCodeForCurrentTimezone();
  CallJS("loadPlayStoreToS", country_code);
}

void ArcTermsOfServiceScreenHandler::OnCurrentScreenChanged(
    OobeScreen current_screen,
    OobeScreen new_screen) {
  if (new_screen != OobeScreen::SCREEN_GAIA_SIGNIN)
    return;

  MaybeLoadPlayStoreToS(false);
  StartNetworkAndTimeZoneObserving();
}

void ArcTermsOfServiceScreenHandler::TimezoneChanged(
    const icu::TimeZone& timezone) {
  MaybeLoadPlayStoreToS(false);
}

void ArcTermsOfServiceScreenHandler::DefaultNetworkChanged(
    const NetworkState* network) {
  MaybeLoadPlayStoreToS(false);
}

void ArcTermsOfServiceScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("arcTermsOfServiceScreenHeading", IDS_ARC_OOBE_TERMS_HEADING);
  builder->Add("arcTermsOfServiceScreenDescription",
      IDS_ARC_OOBE_TERMS_DESCRIPTION);
  builder->Add("arcTermsOfServiceLoading", IDS_ARC_OOBE_TERMS_LOADING);
  builder->Add("arcTermsOfServiceError", IDS_ARC_OOBE_TERMS_LOAD_ERROR);
  builder->Add("arcTermsOfServiceSkipButton", IDS_ARC_OOBE_TERMS_BUTTON_SKIP);
  builder->Add("arcTermsOfServiceRetryButton", IDS_ARC_OOBE_TERMS_BUTTON_RETRY);
  builder->Add("arcTermsOfServiceAcceptButton",
               IDS_ARC_OOBE_TERMS_BUTTON_ACCEPT);
  builder->Add("arcTermsOfServiceNextButton",
               IDS_ARC_OPT_IN_DIALOG_BUTTON_NEXT);
  builder->Add("arcPolicyLink", IDS_ARC_OPT_IN_PRIVACY_POLICY_LINK);
  builder->Add("arcTextBackupRestore", IDS_ARC_OPT_IN_DIALOG_BACKUP_RESTORE);
  builder->Add("arcTextLocationService", IDS_ARC_OPT_IN_LOCATION_SETTING);
  builder->Add("arcTextPaiService", IDS_ARC_OPT_IN_PAI);
  builder->Add("arcTextGoogleServiceConfirmation",
               IDS_ARC_OPT_IN_GOOGLE_SERVICE_CONFIRMATION);
  builder->Add("arcLearnMoreStatistics", IDS_ARC_OPT_IN_LEARN_MORE_STATISTICS);
  builder->Add("arcLearnMoreLocationService",
      IDS_ARC_OPT_IN_LEARN_MORE_LOCATION_SERVICES);
  builder->Add("arcLearnMoreBackupAndRestore",
      IDS_ARC_OPT_IN_LEARN_MORE_BACKUP_AND_RESTORE);
  builder->Add("arcLearnMorePaiService", IDS_ARC_OPT_IN_LEARN_MORE_PAI_SERVICE);
  builder->Add("arcOverlayClose", IDS_ARC_OOBE_TERMS_POPUP_HELP_CLOSE_BUTTON);
  builder->Add("arcOverlayLoading", IDS_ARC_POPUP_HELP_LOADING);
}

void ArcTermsOfServiceScreenHandler::OnMetricsModeChanged(bool enabled,
                                                          bool managed) {
  const Profile* const profile = ProfileManager::GetActiveUserProfile();
  CHECK(profile);

  const user_manager::User* user =
      ProfileHelper::Get()->GetUserByProfile(profile);
  CHECK(user);

  const AccountId owner =
      user_manager::UserManager::Get()->GetOwnerAccountId();

  // Owner may not be set in case of initial account setup. Note, in case of
  // enterprise enrolled devices owner is always empty and we need to account
  // managed flag.
  const bool owner_profile = !owner.is_valid() || user->GetAccountId() == owner;

  if (owner_profile && !managed && !enabled) {
    CallJS("setMetricsMode", base::string16(), false);
  } else {
    int message_id;
    if (owner_profile && !managed) {
      message_id = IDS_ARC_OOBE_TERMS_DIALOG_METRICS_ENABLED;
    } else {
      message_id = enabled ? IDS_ARC_OOBE_TERMS_DIALOG_METRICS_MANAGED_ENABLED
                           : IDS_ARC_OOBE_TERMS_DIALOG_METRICS_MANAGED_DISABLED;
    }
    CallJS("setMetricsMode", l10n_util::GetStringUTF16(message_id), true);
  }
}

void ArcTermsOfServiceScreenHandler::OnBackupAndRestoreModeChanged(
    bool enabled, bool managed) {
  backup_restore_managed_ = managed;
  CallJS("setBackupAndRestoreMode", enabled, managed);
}

void ArcTermsOfServiceScreenHandler::OnLocationServicesModeChanged(
    bool enabled, bool managed) {
  location_services_managed_ = managed;
  CallJS("setLocationServicesMode", enabled, managed);
}

void ArcTermsOfServiceScreenHandler::AddObserver(
    ArcTermsOfServiceScreenViewObserver* observer) {
  observer_list_.AddObserver(observer);
}

void ArcTermsOfServiceScreenHandler::RemoveObserver(
    ArcTermsOfServiceScreenViewObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void ArcTermsOfServiceScreenHandler::Show() {
  if (!page_is_ready()) {
    show_on_init_ = true;
    return;
  }

  DoShow();
}

void ArcTermsOfServiceScreenHandler::Hide() {
  system::TimezoneSettings::GetInstance()->RemoveObserver(this);
  pref_handler_.reset();
}

void ArcTermsOfServiceScreenHandler::StartNetworkAndTimeZoneObserving() {
  if (network_time_zone_observing_)
    return;

  chromeos::NetworkHandler::Get()->network_state_handler()->AddObserver(
      this, FROM_HERE);
  system::TimezoneSettings::GetInstance()->AddObserver(this);
  network_time_zone_observing_ = true;
}

void ArcTermsOfServiceScreenHandler::Initialize() {
  if (!show_on_init_) {
    // Send time zone information as soon as possible to able to pre-load the
    // Play Store ToS.
    GetOobeUI()->AddObserver(this);
    return;
  }

  Show();
  show_on_init_ = false;
}

void ArcTermsOfServiceScreenHandler::DoShow() {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  CHECK(profile);

  // Enable ARC to match ArcSessionManager logic. ArcSessionManager expects that
  // ARC is enabled (prefs::kArcEnabled = true) on showing Terms of Service. If
  // user accepts ToS then prefs::kArcEnabled is left activated. If user skips
  // ToS then prefs::kArcEnabled is automatically reset in ArcSessionManager.
  arc::SetArcPlayStoreEnabledForProfile(profile, true);

  // Hide the Skip button if the ToS screen can not be skipped during OOBE.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kEnableArcOobeOptinNoSkip)) {
    CallJS("hideSkipButton");
  }

  action_taken_ = false;

  ShowScreen(kScreenId);

  arc_managed_ = arc::IsArcPlayStoreEnabledPreferenceManagedForProfile(profile);
  CallJS("setArcManaged", arc_managed_);

  MaybeLoadPlayStoreToS(true);
  StartNetworkAndTimeZoneObserving();

  pref_handler_.reset(new arc::ArcOptInPreferenceHandler(
      this, profile->GetPrefs()));
  pref_handler_->Start();
}

bool ArcTermsOfServiceScreenHandler::NeedDispatchEventOnAction() {
  if (action_taken_)
    return false;
  action_taken_ = true;
  return true;
}

void ArcTermsOfServiceScreenHandler::RecordConsents(
    const std::string& tos_content,
    bool record_tos_content,
    bool tos_accepted,
    bool record_backup_consent,
    bool backup_accepted,
    bool record_location_consent,
    bool location_accepted) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  consent_auditor::ConsentAuditor* consent_auditor =
      ConsentAuditorFactory::GetForProfile(profile);
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile);
  DCHECK(signin_manager->IsAuthenticated());
  const std::string account_id = signin_manager->GetAuthenticatedAccountId();

  // TODO(jhorwich): Replace this approach when passing |is_managed| boolean is
  // supported by the underlying consent protos.
  const std::vector<int> consent_ids = ArcSupportHost::ComputePlayToSConsentIds(
      record_tos_content ? tos_content : "");

  consent_auditor->RecordGaiaConsent(
      account_id, consent_auditor::Feature::PLAY_STORE, consent_ids,
      IDS_ARC_OOBE_TERMS_BUTTON_ACCEPT,
      tos_accepted ? consent_auditor::ConsentStatus::GIVEN
                   : consent_auditor::ConsentStatus::NOT_GIVEN);

  if (record_backup_consent) {
    consent_auditor->RecordGaiaConsent(
        account_id, consent_auditor::Feature::BACKUP_AND_RESTORE,
        {IDS_ARC_OPT_IN_DIALOG_BACKUP_RESTORE},
        IDS_ARC_OOBE_TERMS_BUTTON_ACCEPT,
        backup_accepted ? consent_auditor::ConsentStatus::GIVEN
                        : consent_auditor::ConsentStatus::NOT_GIVEN);
  }

  if (record_location_consent) {
    consent_auditor->RecordGaiaConsent(
        account_id, consent_auditor::Feature::GOOGLE_LOCATION_SERVICE,
        {IDS_ARC_OPT_IN_LOCATION_SETTING}, IDS_ARC_OOBE_TERMS_BUTTON_ACCEPT,
        location_accepted ? consent_auditor::ConsentStatus::GIVEN
                          : consent_auditor::ConsentStatus::NOT_GIVEN);
  }
}

void ArcTermsOfServiceScreenHandler::HandleSkip(
    const std::string& tos_content) {
  if (!NeedDispatchEventOnAction())
    return;

  // Record consents as not accepted for consents that are under user control
  // when the user skips ARC setup.
  RecordConsents(tos_content, !arc_managed_, /*tos_accepted=*/false,
                 !backup_restore_managed_, /*backup_accepted=*/false,
                 !location_services_managed_, /*location_accepted=*/false);

  for (auto& observer : observer_list_)
    observer.OnSkip();
}

void ArcTermsOfServiceScreenHandler::HandleAccept(
    bool enable_backup_restore,
    bool enable_location_services,
    const std::string& tos_content) {
  if (!NeedDispatchEventOnAction())
    return;
  pref_handler_->EnableBackupRestore(enable_backup_restore);
  pref_handler_->EnableLocationService(enable_location_services);

  // Record consents as accepted or not accepted as appropriate for consents
  // that are under user control when the user completes ARC setup.
  RecordConsents(tos_content, !arc_managed_, /*tos_accepted=*/true,
                 !backup_restore_managed_, enable_backup_restore,
                 !location_services_managed_, enable_location_services);

  for (auto& observer : observer_list_)
    observer.OnAccept();
}

}  // namespace chromeos
