// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_FAKE_SYNC_SERVICE_H_
#define COMPONENTS_SYNC_DRIVER_FAKE_SYNC_SERVICE_H_

#include <memory>
#include <string>
#include <utility>

#include "components/signin/core/browser/account_info.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/engine/cycle/sync_cycle_snapshot.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace syncer {

class BaseTransaction;
struct UserShare;

// Fake implementation of SyncService, used for testing.
class FakeSyncService : public SyncService {
 public:
  FakeSyncService();
  ~FakeSyncService() override;

  void set_auth_error(GoogleServiceAuthError error) {
    error_ = std::move(error);
  }

  void SetAuthenticatedAccountInfo(const AccountInfo& account_info);
  AccountInfo GetAuthenticatedAccountInfo() const override;

 private:
  // Dummy methods.
  // SyncService implementation.
  bool IsFirstSetupComplete() const override;
  bool IsSyncAllowed() const override;
  bool IsSyncActive() const override;
  bool IsLocalSyncEnabled() const override;
  void TriggerRefresh(const ModelTypeSet& types) override;
  ModelTypeSet GetActiveDataTypes() const override;
  SyncClient* GetSyncClient() const override;
  void AddObserver(SyncServiceObserver* observer) override;
  void RemoveObserver(SyncServiceObserver* observer) override;
  bool HasObserver(const SyncServiceObserver* observer) const override;
  void OnDataTypeRequestsSyncStartup(ModelType type) override;
  bool CanSyncStart() const override;
  void RequestStop(SyncService::SyncStopDataFate data_fate) override;
  void RequestStart() override;
  ModelTypeSet GetPreferredDataTypes() const override;
  void OnUserChoseDatatypes(bool sync_everything,
                            ModelTypeSet chosen_types) override;
  void SetFirstSetupComplete() override;
  bool IsFirstSetupInProgress() const override;
  std::unique_ptr<SyncSetupInProgressHandle> GetSetupInProgressHandle()
      override;
  bool IsSetupInProgress() const override;
  bool ConfigurationDone() const override;
  const GoogleServiceAuthError& GetAuthError() const override;
  bool HasUnrecoverableError() const override;
  bool IsEngineInitialized() const override;
  sync_sessions::OpenTabsUIDelegate* GetOpenTabsUIDelegate() override;
  bool IsPassphraseRequiredForDecryption() const override;
  base::Time GetExplicitPassphraseTime() const override;
  bool IsUsingSecondaryPassphrase() const override;
  void EnableEncryptEverything() override;
  bool IsEncryptEverythingEnabled() const override;
  void SetEncryptionPassphrase(const std::string& passphrase,
                               PassphraseType type) override;
  bool SetDecryptionPassphrase(const std::string& passphrase) override;
  bool IsCryptographerReady(const BaseTransaction* trans) const override;
  UserShare* GetUserShare() const override;
  LocalDeviceInfoProvider* GetLocalDeviceInfoProvider() const override;
  void RegisterDataTypeController(
      std::unique_ptr<DataTypeController> data_type_controller) override;
  void ReenableDatatype(ModelType type) override;
  SyncTokenStatus GetSyncTokenStatus() const override;
  std::string QuerySyncStatusSummaryString() override;
  bool QueryDetailedSyncStatus(SyncStatus* result) override;
  base::Time GetLastSyncedTime() const override;
  std::string GetEngineInitializationStateString() const override;
  SyncCycleSnapshot GetLastCycleSnapshot() const override;
  std::unique_ptr<base::Value> GetTypeStatusMap() override;
  const GURL& sync_service_url() const override;
  std::string unrecoverable_error_message() const override;
  base::Location unrecoverable_error_location() const override;
  void AddProtocolEventObserver(ProtocolEventObserver* observer) override;
  void RemoveProtocolEventObserver(ProtocolEventObserver* observer) override;
  void AddTypeDebugInfoObserver(TypeDebugInfoObserver* observer) override;
  void RemoveTypeDebugInfoObserver(TypeDebugInfoObserver* observer) override;
  base::WeakPtr<JsController> GetJsController() override;
  void GetAllNodes(const base::Callback<void(std::unique_ptr<base::ListValue>)>&
                       callback) override;
  GlobalIdMapper* GetGlobalIdMapper() const override;

  // DataTypeEncryptionHandler implementation.
  bool IsPassphraseRequired() const override;
  ModelTypeSet GetEncryptedDataTypes() const override;

  // KeyedService implementation.
  void Shutdown() override;

  GoogleServiceAuthError error_;
  GURL sync_service_url_;
  std::string unrecoverable_error_message_;
  std::unique_ptr<UserShare> user_share_;

  AccountInfo account_info_;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_FAKE_SYNC_SERVICE_H_
