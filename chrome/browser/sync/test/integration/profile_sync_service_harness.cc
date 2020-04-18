// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"

#include <cstddef>
#include <iterator>
#include <ostream>
#include <sstream>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind_test_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/test/integration/quiesce_status_change_checker.h"
#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/browser/ui/webui/signin/login_ui_test_utils.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_switches.h"
#include "components/invalidation/impl/p2p_invalidation_service.h"
#include "components/sync/base/progress_marker_map.h"
#include "components/sync/driver/about_sync_util.h"
#include "components/sync/engine/sync_string_conversions.h"
#include "google_apis/gaia/gaia_constants.h"
#include "services/identity/public/cpp/identity_manager.h"

using browser_sync::ProfileSyncService;
using syncer::SyncCycleSnapshot;

namespace {

bool HasAuthError(ProfileSyncService* service) {
  return service->GetAuthError().state() ==
             GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS ||
         service->GetAuthError().state() ==
             GoogleServiceAuthError::SERVICE_ERROR ||
         service->GetAuthError().state() ==
             GoogleServiceAuthError::REQUEST_CANCELED;
}

class EngineInitializeChecker : public SingleClientStatusChangeChecker {
 public:
  explicit EngineInitializeChecker(ProfileSyncService* service)
      : SingleClientStatusChangeChecker(service) {}

  bool IsExitConditionSatisfied() override {
    if (service()->IsEngineInitialized())
      return true;
    // Engine initialization is blocked by an auth error.
    if (HasAuthError(service()))
      return true;
    // Engine initialization is blocked by a failure to fetch Oauth2 tokens.
    if (service()->IsRetryingAccessTokenFetchForTest())
      return true;
    // Still waiting on engine initialization.
    return false;
  }

  std::string GetDebugMessage() const override { return "Engine Initialize"; }
};

class SyncSetupChecker : public SingleClientStatusChangeChecker {
 public:
  explicit SyncSetupChecker(ProfileSyncService* service)
      : SingleClientStatusChangeChecker(service) {}

  bool IsExitConditionSatisfied() override {
    if (!service()->IsSyncActive())
      return false;
    if (service()->ConfigurationDone())
      return true;
    // Sync is blocked because a custom passphrase is required.
    if (service()->passphrase_required_reason() == syncer::REASON_DECRYPTION)
      return true;
    // Sync is blocked by an auth error.
    if (HasAuthError(service()))
      return true;
    // Still waiting on sync setup.
    return false;
  }

  std::string GetDebugMessage() const override { return "Sync Setup"; }
};

}  // namespace

// static
std::unique_ptr<ProfileSyncServiceHarness> ProfileSyncServiceHarness::Create(
    Profile* profile,
    const std::string& username,
    const std::string& gaia_id,
    const std::string& password,
    SigninType signin_type) {
  return base::WrapUnique(new ProfileSyncServiceHarness(
      profile, username, gaia_id, password, signin_type));
}

ProfileSyncServiceHarness::ProfileSyncServiceHarness(
    Profile* profile,
    const std::string& username,
    const std::string& gaia_id,
    const std::string& password,
    SigninType signin_type)
    : profile_(profile),
      service_(ProfileSyncServiceFactory::GetForProfile(profile)),
      username_(username),
      gaia_id_(gaia_id),
      password_(password),
      signin_type_(signin_type),
      oauth2_refesh_token_number_(0),
      profile_debug_name_(profile->GetDebugName()) {}

ProfileSyncServiceHarness::~ProfileSyncServiceHarness() { }

bool ProfileSyncServiceHarness::SetupSync() {
  bool result = SetupSync(syncer::UserSelectableTypes(), false);
  if (!result) {
    LOG(ERROR) << profile_debug_name_ << ": SetupSync failed. Syncer status:\n"
               << GetServiceStatus();
  } else {
    DVLOG(1) << profile_debug_name_ << ": SetupSync successful.";
  }
  return result;
}

bool ProfileSyncServiceHarness::SetupSyncForClearingServerData() {
  bool result = SetupSync(syncer::UserSelectableTypes(), true);
  if (!result) {
    LOG(ERROR) << profile_debug_name_
               << ": SetupSyncForClear failed. Syncer status:\n"
               << GetServiceStatus();
  } else {
    DVLOG(1) << profile_debug_name_ << ": SetupSyncForClear successful.";
  }
  return result;
}

bool ProfileSyncServiceHarness::SetupSync(syncer::ModelTypeSet synced_datatypes,
                                          bool skip_passphrase_verification) {
  DCHECK(!profile_->IsLegacySupervised())
      << "SetupSync should not be used for legacy supervised users.";

  // Initialize the sync client's profile sync service object.
  if (service() == nullptr) {
    LOG(ERROR) << "SetupSync(): service() is null.";
    return false;
  }

  // Tell the sync service that setup is in progress so we don't start syncing
  // until we've finished configuration.
  sync_blocker_ = service()->GetSetupInProgressHandle();

  DCHECK(!username_.empty());
  if (signin_type_ == SigninType::UI_SIGNIN) {
    Browser* browser = chrome::FindBrowserWithProfile(profile_);
    DCHECK(browser);
    if (!login_ui_test_utils::SignInWithUI(browser, username_, password_)) {
      LOG(ERROR) << "Could not sign in to GAIA servers.";
      return false;
    }
  } else if (signin_type_ == SigninType::FAKE_SIGNIN) {
    // Authenticate sync client using GAIA credentials.
    // TODO(https://crbug.com/814307): This ideally should go through
    // identity_test_utils.h (and in the long run IdentityTestEnvironment), but
    // making that change is complex for reasons described in the bug.
    identity::IdentityManager* identity_manager =
        IdentityManagerFactory::GetForProfile(profile_);
    identity_manager->SetPrimaryAccountSynchronouslyForTests(
        gaia_id_, username_, GenerateFakeOAuth2RefreshTokenString());
  } else {
    LOG(ERROR) << "Unsupported profile signin type.";
  }

  // Now that auth is completed, request that sync actually start.
  service()->RequestStart();

  if (!AwaitEngineInitialization(skip_passphrase_verification)) {
    return false;
  }
  // Choose the datatypes to be synced. If all datatypes are to be synced,
  // set sync_everything to true; otherwise, set it to false.
  bool sync_everything = (synced_datatypes == syncer::UserSelectableTypes());
  service()->OnUserChoseDatatypes(sync_everything, synced_datatypes);

  // Notify ProfileSyncService that we are done with configuration.
  if (skip_passphrase_verification) {
    sync_blocker_.reset();
  } else {
    FinishSyncSetup();
  }

  if ((signin_type_ == SigninType::UI_SIGNIN) &&
      !login_ui_test_utils::DismissSyncConfirmationDialog(
          chrome::FindBrowserWithProfile(profile_),
          base::TimeDelta::FromSeconds(30))) {
    LOG(ERROR) << "Failed to dismiss sync confirmation dialog.";
    return false;
  }

  // OneClickSigninSyncStarter observer is created with a real user sign in.
  // It is deleted on certain conditions which are not satisfied by our tests,
  // and this causes the SigninTracker observer to stay hanging at shutdown.
  // Calling LoginUIService::SyncConfirmationUIClosed forces the observer to
  // be removed. http://crbug.com/484388
  if (signin_type_ == SigninType::UI_SIGNIN) {
    LoginUIServiceFactory::GetForProfile(profile_)->SyncConfirmationUIClosed(
        LoginUIService::SYNC_WITH_DEFAULT_SETTINGS);
  }

  if (!skip_passphrase_verification) {
    // Set an implicit passphrase for encryption if an explicit one hasn't
    // already been set. If an explicit passphrase has been set, immediately
    // return false, since a decryption passphrase is required.
    if (!service()->IsUsingSecondaryPassphrase()) {
      service()->SetEncryptionPassphrase(password_,
                                         ProfileSyncService::IMPLICIT);
    } else {
      LOG(ERROR)
          << "A passphrase is required for decryption. Sync cannot proceed"
             " until SetDecryptionPassphrase is called.";
      return false;
    }
  }

  // Wait for initial sync cycle to be completed.
  if (!AwaitSyncSetupCompletion(skip_passphrase_verification)) {
    return false;
  }

  return true;
}

void ProfileSyncServiceHarness::StopSyncService(
    syncer::SyncService::SyncStopDataFate data_fate) {
  DVLOG(1) << "Requesting stop for service.";
  service()->RequestStop(data_fate);
}

bool ProfileSyncServiceHarness::StartSyncService() {
  std::unique_ptr<syncer::SyncSetupInProgressHandle> blocker =
      service()->GetSetupInProgressHandle();
  DVLOG(1) << "Requesting start for service";
  service()->RequestStart();

  if (!AwaitEngineInitialization(/*skip_passphrase_verification=*/false)) {
    LOG(ERROR) << "AwaitEngineInitialization failed.";
    return false;
  }
  DVLOG(1) << "Engine Initialized successfully.";

  // This passphrase should be implicit because ClearServerData should be called
  // prior.
  if (!service()->IsUsingSecondaryPassphrase()) {
    service()->SetEncryptionPassphrase(password_, ProfileSyncService::IMPLICIT);
  } else {
    LOG(ERROR) << "A passphrase is required for decryption. Sync cannot proceed"
                  " until SetDecryptionPassphrase is called.";
    return false;
  }
  DVLOG(1) << "Passphrase decryption success.";

  blocker.reset();
  service()->SetFirstSetupComplete();

  if (!AwaitSyncSetupCompletion(/*skip_passphrase_verification=*/false)) {
    LOG(FATAL) << "AwaitSyncSetupCompletion failed.";
    return false;
  }

  return true;
}

void ProfileSyncServiceHarness::SignoutSyncService() {
  DCHECK(!username_.empty());
  service()->OnPrimaryAccountCleared();
}

bool ProfileSyncServiceHarness::HasUnsyncedItems() {
  base::RunLoop loop;
  bool result = false;
  service()->HasUnsyncedItemsForTest(
      base::BindLambdaForTesting([&](bool has_unsynced_items) {
        result = has_unsynced_items;
        loop.Quit();
      }));
  loop.Run();
  return result;
}

bool ProfileSyncServiceHarness::AwaitMutualSyncCycleCompletion(
    ProfileSyncServiceHarness* partner) {
  std::vector<ProfileSyncServiceHarness*> harnesses;
  harnesses.push_back(this);
  harnesses.push_back(partner);
  return AwaitQuiescence(harnesses);
}

bool ProfileSyncServiceHarness::AwaitGroupSyncCycleCompletion(
    const std::vector<ProfileSyncServiceHarness*>& partners) {
  return AwaitQuiescence(partners);
}

// static
bool ProfileSyncServiceHarness::AwaitQuiescence(
    const std::vector<ProfileSyncServiceHarness*>& clients) {
  std::vector<ProfileSyncService*> services;
  if (clients.empty()) {
    return true;
  }

  for (const ProfileSyncServiceHarness* harness : clients) {
    services.push_back(harness->service());
  }
  return QuiesceStatusChangeChecker(services).Wait();
}

bool ProfileSyncServiceHarness::AwaitEngineInitialization(
    bool skip_passphrase_verification) {
  if (!EngineInitializeChecker(service()).Wait()) {
    LOG(ERROR) << "EngineInitializeChecker timed out.";
    return false;
  }

  if (!service()->IsEngineInitialized()) {
    LOG(ERROR) << "Service engine not initialized.";
    return false;
  }

  // Make sure that initial sync wasn't blocked by a missing passphrase.
  if (!skip_passphrase_verification &&
      service()->passphrase_required_reason() == syncer::REASON_DECRYPTION) {
    LOG(ERROR) << "A passphrase is required for decryption. Sync cannot proceed"
                  " until SetDecryptionPassphrase is called.";
    return false;
  }

  if (HasAuthError(service())) {
    LOG(ERROR) << "Credentials were rejected. Sync cannot proceed.";
    return false;
  }

  return true;
}

bool ProfileSyncServiceHarness::AwaitSyncSetupCompletion(
    bool skip_passphrase_verification) {
  if (!SyncSetupChecker(service()).Wait()) {
    LOG(ERROR) << "SyncSetupChecker timed out.";
    return false;
  }

  // If passphrase verification is not skipped, make sure that initial sync
  // wasn't blocked by a missing passphrase.
  if (!skip_passphrase_verification &&
      service()->passphrase_required_reason() == syncer::REASON_DECRYPTION) {
    LOG(ERROR) << "A passphrase is required for decryption. Sync cannot proceed"
                  " until SetDecryptionPassphrase is called.";
    return false;
  }

  if (HasAuthError(service())) {
    LOG(ERROR) << "Credentials were rejected. Sync cannot proceed.";
    return false;
  }

  return true;
}

std::string ProfileSyncServiceHarness::GenerateFakeOAuth2RefreshTokenString() {
  return base::StringPrintf("oauth2_refresh_token_%d",
                            ++oauth2_refesh_token_number_);
}

bool ProfileSyncServiceHarness::IsSyncDisabled() const {
  return !service()->IsSetupInProgress() && !service()->IsFirstSetupComplete();
}

void ProfileSyncServiceHarness::FinishSyncSetup() {
  sync_blocker_.reset();
  service()->SetFirstSetupComplete();
}

SyncCycleSnapshot ProfileSyncServiceHarness::GetLastCycleSnapshot() const {
  DCHECK(service() != nullptr) << "Sync service has not yet been set up.";
  if (service()->IsSyncActive()) {
    return service()->GetLastCycleSnapshot();
  }
  return SyncCycleSnapshot();
}

bool ProfileSyncServiceHarness::EnableSyncForDatatype(
    syncer::ModelType datatype) {
  DVLOG(1) << GetClientInfoString(
      "EnableSyncForDatatype("
      + std::string(syncer::ModelTypeToString(datatype)) + ")");

  if (IsSyncDisabled())
    return SetupSync(syncer::ModelTypeSet(datatype));

  if (service() == nullptr) {
    LOG(ERROR) << "EnableSyncForDatatype(): service() is null.";
    return false;
  }

  if (!syncer::UserSelectableTypes().Has(datatype)) {
    LOG(ERROR) << "Can only enable user selectable types, requested "
               << syncer::ModelTypeToString(datatype);
    return false;
  }

  syncer::ModelTypeSet synced_datatypes = service()->GetPreferredDataTypes();
  if (synced_datatypes.Has(datatype)) {
    DVLOG(1) << "EnableSyncForDatatype(): Sync already enabled for datatype "
             << syncer::ModelTypeToString(datatype)
             << " on " << profile_debug_name_ << ".";
    return true;
  }

  synced_datatypes.Put(syncer::ModelTypeFromInt(datatype));
  synced_datatypes.RetainAll(syncer::UserSelectableTypes());
  service()->OnUserChoseDatatypes(false, synced_datatypes);
  if (AwaitSyncSetupCompletion(/*skip_passphrase_verification=*/false)) {
    DVLOG(1) << "EnableSyncForDatatype(): Enabled sync for datatype "
             << syncer::ModelTypeToString(datatype)
             << " on " << profile_debug_name_ << ".";
    return true;
  }

  DVLOG(0) << GetClientInfoString("EnableSyncForDatatype failed");
  return false;
}

bool ProfileSyncServiceHarness::DisableSyncForDatatype(
    syncer::ModelType datatype) {
  DVLOG(1) << GetClientInfoString(
      "DisableSyncForDatatype("
      + std::string(syncer::ModelTypeToString(datatype)) + ")");

  if (service() == nullptr) {
    LOG(ERROR) << "DisableSyncForDatatype(): service() is null.";
    return false;
  }

  if (!syncer::UserSelectableTypes().Has(datatype)) {
    LOG(ERROR) << "Can only disable user selectable types, requested "
               << syncer::ModelTypeToString(datatype);
    return false;
  }

  syncer::ModelTypeSet synced_datatypes = service()->GetPreferredDataTypes();
  if (!synced_datatypes.Has(datatype)) {
    DVLOG(1) << "DisableSyncForDatatype(): Sync already disabled for datatype "
             << syncer::ModelTypeToString(datatype)
             << " on " << profile_debug_name_ << ".";
    return true;
  }

  synced_datatypes.RetainAll(syncer::UserSelectableTypes());
  synced_datatypes.Remove(datatype);
  service()->OnUserChoseDatatypes(false, synced_datatypes);
  if (AwaitSyncSetupCompletion(/*skip_passphrase_verification=*/false)) {
    DVLOG(1) << "DisableSyncForDatatype(): Disabled sync for datatype "
             << syncer::ModelTypeToString(datatype)
             << " on " << profile_debug_name_ << ".";
    return true;
  }

  DVLOG(0) << GetClientInfoString("DisableSyncForDatatype failed");
  return false;
}

bool ProfileSyncServiceHarness::EnableSyncForAllDatatypes() {
  DVLOG(1) << GetClientInfoString("EnableSyncForAllDatatypes");

  if (IsSyncDisabled())
    return SetupSync();

  if (service() == nullptr) {
    LOG(ERROR) << "EnableSyncForAllDatatypes(): service() is null.";
    return false;
  }

  service()->OnUserChoseDatatypes(true, syncer::UserSelectableTypes());
  if (AwaitSyncSetupCompletion(/*skip_passphrase_verification=*/false)) {
    DVLOG(1) << "EnableSyncForAllDatatypes(): Enabled sync for all datatypes "
             << "on " << profile_debug_name_ << ".";
    return true;
  }

  DVLOG(0) << GetClientInfoString("EnableSyncForAllDatatypes failed");
  return false;
}

bool ProfileSyncServiceHarness::DisableSyncForAllDatatypes() {
  DVLOG(1) << GetClientInfoString("DisableSyncForAllDatatypes");

  if (service() == nullptr) {
    LOG(ERROR) << "DisableSyncForAllDatatypes(): service() is null.";
    return false;
  }

  service()->RequestStop(ProfileSyncService::CLEAR_DATA);

  DVLOG(1) << "DisableSyncForAllDatatypes(): Disabled sync for all "
           << "datatypes on " << profile_debug_name_;
  return true;
}

// TODO(sync): Clean up this method in a separate CL. Remove all snapshot fields
// and log shorter, more meaningful messages.
std::string ProfileSyncServiceHarness::GetClientInfoString(
    const std::string& message) const {
  std::stringstream os;
  os << profile_debug_name_ << ": " << message << ": ";
  if (service()) {
    const SyncCycleSnapshot& snap = GetLastCycleSnapshot();
    syncer::SyncStatus status;
    service()->QueryDetailedSyncStatus(&status);
    // Capture select info from the sync session snapshot and syncer status.
    os << ", has_unsynced_items: " << snap.has_remaining_local_changes()
       << ", did_commit: "
       << (snap.model_neutral_state().num_successful_commits == 0 &&
           snap.model_neutral_state().commit_result == syncer::SYNCER_OK)
       << ", encryption conflicts: " << snap.num_encryption_conflicts()
       << ", hierarchy conflicts: " << snap.num_hierarchy_conflicts()
       << ", server conflicts: " << snap.num_server_conflicts()
       << ", num_updates_downloaded : "
       << snap.model_neutral_state().num_updates_downloaded_total
       << ", passphrase_required_reason: "
       << syncer::PassphraseRequiredReasonToString(
              service()->passphrase_required_reason())
       << ", notifications_enabled: " << status.notifications_enabled
       << ", service_is_active: " << service()->IsSyncActive();
  } else {
    os << "Sync service not available";
  }
  return os.str();
}

bool ProfileSyncServiceHarness::IsTypePreferred(syncer::ModelType type) {
  return service()->GetPreferredDataTypes().Has(type);
}

std::string ProfileSyncServiceHarness::GetServiceStatus() {
  std::unique_ptr<base::DictionaryValue> value(
      syncer::sync_ui_util::ConstructAboutInformation(service(),
                                                      chrome::GetChannel()));
  std::string service_status;
  base::JSONWriter::WriteWithOptions(
      *value, base::JSONWriter::OPTIONS_PRETTY_PRINT, &service_status);
  return service_status;
}
