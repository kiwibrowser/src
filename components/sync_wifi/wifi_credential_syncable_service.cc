// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_credential_syncable_service.h"

#include <stdint.h>

#include <utility>
#include <vector>

#include "base/logging.h"
#include "components/sync/model/sync_change.h"
#include "components/sync/model/sync_data.h"
#include "components/sync/model/sync_error.h"
#include "components/sync/model/sync_error_factory.h"
#include "components/sync/model/sync_merge_result.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync_wifi/wifi_credential.h"
#include "components/sync_wifi/wifi_security_class.h"

namespace sync_wifi {

namespace {

struct RawCredentialData {
  std::vector<uint8_t> ssid;
  WifiSecurityClass security_class;
  std::string passphrase;
};

void BuildSpecifics(const WifiCredential& credential,
                    sync_pb::EntitySpecifics* out_buffer) {
  DCHECK(out_buffer);
  sync_pb::WifiCredentialSpecifics* credential_specifics =
      out_buffer->mutable_wifi_credential();
  DCHECK(credential_specifics);
  credential_specifics->set_ssid(credential.ssid().data(),
                                 credential.ssid().size());
  credential_specifics->set_security_class(
      WifiSecurityClassToSyncSecurityClass(credential.security_class()));
  if (WifiSecurityClassSupportsPassphrases(credential.security_class())) {
    credential_specifics->set_passphrase(credential.passphrase().data(),
                                         credential.passphrase().size());
  }
}

bool ParseSpecifics(const sync_pb::EntitySpecifics& specifics,
                    RawCredentialData* raw_credential) {
  DCHECK(raw_credential);
  if (!specifics.has_wifi_credential()) {
    LOG(ERROR) << "Specifics with missing wifi_credential; skipping";
    return false;
  }

  const sync_pb::WifiCredentialSpecifics& credential_specifics =
      specifics.wifi_credential();
  if (!credential_specifics.has_ssid()) {
    LOG(ERROR) << "Specifics with missing SSID; skipping";
    return false;
  }
  if (!credential_specifics.has_security_class()) {
    LOG(ERROR) << "Specifics with missing security class; skipping";
    return false;
  }

  const WifiSecurityClass security_class =
      WifiSecurityClassFromSyncSecurityClass(
          credential_specifics.security_class());
  if (WifiSecurityClassSupportsPassphrases(security_class) &&
      !credential_specifics.has_passphrase()) {
    LOG(ERROR) << "Specifics for security class "
               << credential_specifics.security_class()
               << " is missing passphrase; skipping";
    return false;
  }

  raw_credential->ssid.assign(credential_specifics.ssid().begin(),
                              credential_specifics.ssid().end());
  raw_credential->security_class = security_class;
  raw_credential->passphrase = credential_specifics.passphrase();
  return true;
}

// TODO(quiche): Separate SyncData validation from parsing of
// WifiCredentialSpecifics.
bool ParseSyncData(const syncer::SyncData& sync_data,
                   RawCredentialData* raw_credential) {
  DCHECK(raw_credential);
  if (!sync_data.IsValid()) {
    LOG(WARNING) << "Invalid SyncData; skipping item";
    return false;
  }

  if (sync_data.GetDataType() != syncer::WIFI_CREDENTIALS) {
    LOG(WARNING) << "Unexpected SyncData of type "
                 << syncer::ModelTypeToString(sync_data.GetDataType())
                 << "; skipping item";
    return false;
  }

  return ParseSpecifics(sync_data.GetSpecifics(), raw_credential);
}

}  // namespace

const syncer::ModelType WifiCredentialSyncableService::kModelType =
    syncer::WIFI_CREDENTIALS;

WifiCredentialSyncableService::WifiCredentialSyncableService(
    std::unique_ptr<WifiConfigDelegate> network_config_delegate)
    : network_config_delegate_(std::move(network_config_delegate)) {
  DCHECK(network_config_delegate_);
}

WifiCredentialSyncableService::~WifiCredentialSyncableService() {}

syncer::SyncMergeResult WifiCredentialSyncableService::MergeDataAndStartSyncing(
    syncer::ModelType type,
    const syncer::SyncDataList& initial_sync_data,
    std::unique_ptr<syncer::SyncChangeProcessor> sync_processor,
    std::unique_ptr<syncer::SyncErrorFactory> /* error_handler */) {
  DCHECK(!sync_processor_.get());
  DCHECK(sync_processor.get());
  DCHECK_EQ(kModelType, type);

  sync_processor_ = std::move(sync_processor);

  // TODO(quiche): Update local WiFi configuration from |initial_sync_data|.
  // TODO(quiche): Notify upper layers that sync is ready.
  NOTIMPLEMENTED();

  return syncer::SyncMergeResult(type);
}

void WifiCredentialSyncableService::StopSyncing(syncer::ModelType type) {
  DCHECK_EQ(kModelType, type);
  sync_processor_.reset();
}

syncer::SyncDataList WifiCredentialSyncableService::GetAllSyncData(
    syncer::ModelType type) const {
  DCHECK_EQ(kModelType, type);
  NOTIMPLEMENTED();
  return syncer::SyncDataList();
}

syncer::SyncError WifiCredentialSyncableService::ProcessSyncChanges(
    const base::Location& /* caller_location */,
    const syncer::SyncChangeList& change_list) {
  if (!sync_processor_.get()) {
    return syncer::SyncError(
        FROM_HERE, syncer::SyncError::UNREADY_ERROR,
        "ProcessSyncChanges called before MergeDataAndStartSyncing",
        kModelType);
  }

  for (const syncer::SyncChange& sync_change : change_list) {
    DCHECK(sync_change.IsValid());
    RawCredentialData raw_credential;
    if (!ParseSyncData(sync_change.sync_data(), &raw_credential)) {
      LOG(WARNING) << "Failed to parse item; skipping "
                   << syncer::SyncChange::ChangeTypeToString(
                          sync_change.change_type());
      continue;
    }

    std::unique_ptr<WifiCredential> credential;
    switch (sync_change.change_type()) {
      case syncer::SyncChange::ACTION_ADD:
        credential = WifiCredential::Create(raw_credential.ssid,
                                            raw_credential.security_class,
                                            raw_credential.passphrase);
        if (!credential)
          LOG(WARNING) << "Failed to create credential; skipping";
        else
          network_config_delegate_->AddToLocalNetworks(*credential);
        break;
      case syncer::SyncChange::ACTION_UPDATE:
        // TODO(quiche): Implement update, and add appropriate tests.
        NOTIMPLEMENTED();
        break;
      case syncer::SyncChange::ACTION_DELETE:
        // TODO(quiche): Implement delete, and add appropriate tests.
        NOTIMPLEMENTED();
        break;
      default:
        return syncer::SyncError(
            FROM_HERE, syncer::SyncError::DATATYPE_ERROR,
            "ProcessSyncChanges given invalid SyncChangeType", kModelType);
    }
  }

  return syncer::SyncError();
}

bool WifiCredentialSyncableService::AddToSyncedNetworks(
    const std::string& item_id,
    const WifiCredential& credential) {
  if (!sync_processor_.get()) {
    // Callers must queue updates until MergeDataAndStartSyncing has
    // been called on this SyncableService.
    LOG(WARNING) << "WifiCredentials syncable service is not started.";
    return false;
  }

  const SsidAndSecurityClass network_id(credential.ssid(),
                                        credential.security_class());
  if (synced_networks_and_passphrases_.find(network_id) !=
      synced_networks_and_passphrases_.end()) {
    // TODO(quiche): If passphrase has changed, submit this to sync as
    // an ACTION_UPDATE. crbug.com/431436
    return false;
  }

  syncer::SyncChangeList change_list;
  syncer::SyncError sync_error;
  sync_pb::EntitySpecifics wifi_credential_specifics;
  BuildSpecifics(credential, &wifi_credential_specifics);
  change_list.push_back(
      syncer::SyncChange(FROM_HERE, syncer::SyncChange::ACTION_ADD,
                         syncer::SyncData::CreateLocalData(
                             item_id, item_id, wifi_credential_specifics)));
  sync_error = sync_processor_->ProcessSyncChanges(FROM_HERE, change_list);
  if (sync_error.IsSet()) {
    LOG(ERROR) << sync_error.ToString();
    return false;
  }

  synced_networks_and_passphrases_[network_id] = credential.passphrase();
  return true;
}

}  // namespace sync_wifi
