// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WALLET_SYNCABLE_SERVICE_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WALLET_SYNCABLE_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "base/threading/thread_checker.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/sync/model/syncable_service.h"

namespace autofill {

class AutofillTable;
class AutofillWebDataBackend;
class AutofillWebDataService;

// Syncs masked cards (last 4 digits only) and addresses from the sync user's
// Wallet account.
class AutofillWalletSyncableService
    : public base::SupportsUserData::Data,
      public syncer::SyncableService {
 public:
  ~AutofillWalletSyncableService() override;

  // syncer::SyncableService implementation.
  syncer::SyncMergeResult MergeDataAndStartSyncing(
      syncer::ModelType type,
      const syncer::SyncDataList& initial_sync_data,
      std::unique_ptr<syncer::SyncChangeProcessor> sync_processor,
      std::unique_ptr<syncer::SyncErrorFactory> sync_error_factory) override;
  void StopSyncing(syncer::ModelType type) override;
  syncer::SyncDataList GetAllSyncData(syncer::ModelType type) const override;
  syncer::SyncError ProcessSyncChanges(
      const base::Location& from_here,
      const syncer::SyncChangeList& change_list) override;

  // Creates a new AutofillWalletSyncableService and hangs it off of
  // |web_data_service|, which takes ownership. This method should only be
  // called on |web_data_service|'s DB thread.
  static void CreateForWebDataServiceAndBackend(
      AutofillWebDataService* web_data_service,
      AutofillWebDataBackend* webdata_backend,
      const std::string& app_locale);

  // Retrieves the AutofillWalletSyncableService stored on |web_data_service|.
  static AutofillWalletSyncableService* FromWebDataService(
      AutofillWebDataService* web_data_service);

  // Provides a StartSyncFlare to the SyncableService. See
  // sync_start_util for more.
  void InjectStartSyncFlare(
      const syncer::SyncableService::StartSyncFlare& flare);

 protected:
  AutofillWalletSyncableService(
      AutofillWebDataBackend* webdata_backend,
      const std::string& app_locale);

 private:
  FRIEND_TEST_ALL_PREFIXES(AutofillWalletSyncableServiceTest,
                           CopyRelevantMetadataFromDisk_KeepLocalAddresses);
  FRIEND_TEST_ALL_PREFIXES(
      AutofillWalletSyncableServiceTest,
      CopyRelevantMetadataFromDisk_OverwriteOtherAddresses);
  FRIEND_TEST_ALL_PREFIXES(
      AutofillWalletSyncableServiceTest,
      PopulateWalletCardsAndAddresses_BillingAddressIdTransfer);
  FRIEND_TEST_ALL_PREFIXES(AutofillWalletSyncableServiceTest,
                           CopyRelevantMetadataFromDisk_KeepUseStats);
  FRIEND_TEST_ALL_PREFIXES(AutofillWalletSyncableServiceTest, NewWalletCard);
  FRIEND_TEST_ALL_PREFIXES(AutofillWalletSyncableServiceTest, EmptyNameOnCard);
  FRIEND_TEST_ALL_PREFIXES(AutofillWalletSyncableServiceTest, ComputeCardsDiff);
  FRIEND_TEST_ALL_PREFIXES(AutofillWalletSyncableServiceTest,
                           ComputeAddressesDiff);

  struct Diff {
    int items_added = 0;
    int items_removed = 0;

    bool IsEmpty() const { return items_added == 0 && items_removed == 0; }
  };

  // Computes a "diff" (items added, items removed) of two vectors of items,
  // which should be either CreditCard or AutofillProfile. This is used for two
  // purposes:
  // 1) Detecting if anything has changed, so that we don't write to disk in the
  //    common case where nothing has changed.
  // 2) Recording metrics on the number of added/removed items.
  // This is exposed as a static method so that it can be tested.
  template <class Item>
  static Diff ComputeDiff(const std::vector<std::unique_ptr<Item>>& old_data,
                          const std::vector<Item>& new_data);

  syncer::SyncMergeResult SetSyncData(const syncer::SyncDataList& data_list,
                                      bool is_initial_data);

  // Populates the wallet cards and addresses from the sync data and uses the
  // sync data to link the card to its billing address.
  static void PopulateWalletCardsAndAddresses(
      const syncer::SyncDataList& data_list,
      std::vector<CreditCard>* wallet_cards,
      std::vector<AutofillProfile>* wallet_addresses);

  // Finds the copies of the same credit card from the server and on disk and
  // overwrites the server version with the use stats saved on disk, and the
  // billing id if it refers to a local autofill profile. The credit card's IDs
  // do not change over time.
  static void CopyRelevantMetadataFromDisk(
      const AutofillTable& table,
      std::vector<CreditCard>* cards_from_server);

  base::ThreadChecker thread_checker_;

  AutofillWebDataBackend* webdata_backend_;  // Weak ref.

  std::unique_ptr<syncer::SyncChangeProcessor> sync_processor_;

  syncer::SyncableService::StartSyncFlare flare_;

  DISALLOW_COPY_AND_ASSIGN(AutofillWalletSyncableService);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WALLET_SYNCABLE_SERVICE_H_
