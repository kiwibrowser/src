// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/fake_sync_service.h"

#include "base/values.h"
#include "components/signin/core/browser/account_info.h"
#include "components/sync/driver/data_type_controller.h"
#include "components/sync/driver/sync_token_status.h"
#include "components/sync/syncable/base_transaction.h"
#include "components/sync/syncable/user_share.h"

namespace syncer {

void FakeSyncService::SetAuthenticatedAccountInfo(
    const AccountInfo& account_info) {
  account_info_ = account_info;
}
AccountInfo FakeSyncService::GetAuthenticatedAccountInfo() const {
  return account_info_;
}

// Dummy methods

FakeSyncService::FakeSyncService()
    : error_(GoogleServiceAuthError::NONE),
      user_share_(std::make_unique<UserShare>()) {}

FakeSyncService::~FakeSyncService() {}

bool FakeSyncService::IsFirstSetupComplete() const {
  return false;
}

bool FakeSyncService::IsSyncAllowed() const {
  return false;
}

bool FakeSyncService::IsSyncActive() const {
  return false;
}

bool FakeSyncService::IsLocalSyncEnabled() const {
  return false;
}

void FakeSyncService::TriggerRefresh(const ModelTypeSet& types) {}

ModelTypeSet FakeSyncService::GetActiveDataTypes() const {
  return ModelTypeSet();
}

SyncClient* FakeSyncService::GetSyncClient() const {
  return nullptr;
}

void FakeSyncService::AddObserver(SyncServiceObserver* observer) {}

void FakeSyncService::RemoveObserver(SyncServiceObserver* observer) {}

bool FakeSyncService::HasObserver(const SyncServiceObserver* observer) const {
  return false;
}

bool FakeSyncService::CanSyncStart() const {
  return false;
}

void FakeSyncService::OnDataTypeRequestsSyncStartup(ModelType type) {}

void FakeSyncService::RequestStop(SyncService::SyncStopDataFate data_fate) {}

void FakeSyncService::RequestStart() {}

ModelTypeSet FakeSyncService::GetPreferredDataTypes() const {
  return ModelTypeSet();
}

void FakeSyncService::OnUserChoseDatatypes(bool sync_everything,
                                           ModelTypeSet chosen_types) {}

void FakeSyncService::SetFirstSetupComplete() {}

bool FakeSyncService::IsFirstSetupInProgress() const {
  return false;
}

std::unique_ptr<SyncSetupInProgressHandle>
FakeSyncService::GetSetupInProgressHandle() {
  return nullptr;
}

bool FakeSyncService::IsSetupInProgress() const {
  return false;
}

bool FakeSyncService::ConfigurationDone() const {
  return false;
}

const GoogleServiceAuthError& FakeSyncService::GetAuthError() const {
  return error_;
}

bool FakeSyncService::HasUnrecoverableError() const {
  return false;
}

bool FakeSyncService::IsEngineInitialized() const {
  return false;
}

sync_sessions::OpenTabsUIDelegate* FakeSyncService::GetOpenTabsUIDelegate() {
  return nullptr;
}

bool FakeSyncService::IsPassphraseRequiredForDecryption() const {
  return false;
}

base::Time FakeSyncService::GetExplicitPassphraseTime() const {
  return base::Time();
}

bool FakeSyncService::IsUsingSecondaryPassphrase() const {
  return false;
}

void FakeSyncService::EnableEncryptEverything() {}

bool FakeSyncService::IsEncryptEverythingEnabled() const {
  return false;
}

void FakeSyncService::SetEncryptionPassphrase(const std::string& passphrase,
                                              PassphraseType type) {}

bool FakeSyncService::SetDecryptionPassphrase(const std::string& passphrase) {
  return false;
}

bool FakeSyncService::IsCryptographerReady(const BaseTransaction* trans) const {
  return false;
}

UserShare* FakeSyncService::GetUserShare() const {
  return user_share_.get();
}

LocalDeviceInfoProvider* FakeSyncService::GetLocalDeviceInfoProvider() const {
  return nullptr;
}

void FakeSyncService::RegisterDataTypeController(
    std::unique_ptr<DataTypeController> data_type_controller) {}

void FakeSyncService::ReenableDatatype(ModelType type) {}

syncer::SyncTokenStatus FakeSyncService::GetSyncTokenStatus() const {
  return syncer::SyncTokenStatus();
}

std::string FakeSyncService::QuerySyncStatusSummaryString() {
  return "";
}

bool FakeSyncService::QueryDetailedSyncStatus(SyncStatus* result) {
  return false;
}

base::Time FakeSyncService::GetLastSyncedTime() const {
  return base::Time();
}

std::string FakeSyncService::GetEngineInitializationStateString() const {
  return std::string();
}

SyncCycleSnapshot FakeSyncService::GetLastCycleSnapshot() const {
  return SyncCycleSnapshot();
}

std::unique_ptr<base::Value> FakeSyncService::GetTypeStatusMap() {
  return std::make_unique<base::ListValue>();
}

const GURL& FakeSyncService::sync_service_url() const {
  return sync_service_url_;
}

std::string FakeSyncService::unrecoverable_error_message() const {
  return unrecoverable_error_message_;
}

base::Location FakeSyncService::unrecoverable_error_location() const {
  return base::Location();
}

void FakeSyncService::AddProtocolEventObserver(
    ProtocolEventObserver* observer) {}

void FakeSyncService::RemoveProtocolEventObserver(
    ProtocolEventObserver* observer) {}

void FakeSyncService::AddTypeDebugInfoObserver(
    TypeDebugInfoObserver* observer) {}

void FakeSyncService::RemoveTypeDebugInfoObserver(
    TypeDebugInfoObserver* observer) {}

base::WeakPtr<JsController> FakeSyncService::GetJsController() {
  return base::WeakPtr<JsController>();
}

void FakeSyncService::GetAllNodes(
    const base::Callback<void(std::unique_ptr<base::ListValue>)>& callback) {}

GlobalIdMapper* FakeSyncService::GetGlobalIdMapper() const {
  return nullptr;
}

bool FakeSyncService::IsPassphraseRequired() const {
  return false;
}

ModelTypeSet FakeSyncService::GetEncryptedDataTypes() const {
  return ModelTypeSet();
}

void FakeSyncService::Shutdown() {}

}  // namespace syncer
