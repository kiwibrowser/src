// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/webdata/autofill_wallet_syncable_service.h"

#include <memory>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/test_autofill_clock.h"
#include "components/autofill/core/browser/webdata/autofill_table.h"
#include "components/sync/protocol/autofill_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {
namespace {

syncer::SyncData CreateSyncDataForWalletCreditCard(
    const std::string& id,
    const std::string& billing_address_id) {
  sync_pb::EntitySpecifics specifics;

  sync_pb::AutofillWalletSpecifics* wallet_specifics =
      specifics.mutable_autofill_wallet();
  wallet_specifics->set_type(
      sync_pb::AutofillWalletSpecifics_WalletInfoType::
          AutofillWalletSpecifics_WalletInfoType_MASKED_CREDIT_CARD);

  sync_pb::WalletMaskedCreditCard* card_specifics =
      wallet_specifics->mutable_masked_card();
  card_specifics->set_id(id);
  card_specifics->set_billing_address_id(billing_address_id);
  return syncer::SyncData::CreateLocalData(id, id, specifics);
}

syncer::SyncData CreateSyncDataForWalletAddress(const std::string& id) {
  sync_pb::EntitySpecifics specifics;

  sync_pb::AutofillWalletSpecifics* wallet_specifics =
      specifics.mutable_autofill_wallet();
  wallet_specifics->set_type(
      sync_pb::AutofillWalletSpecifics_WalletInfoType::
          AutofillWalletSpecifics_WalletInfoType_POSTAL_ADDRESS);

  sync_pb::WalletPostalAddress* address_specifics =
      wallet_specifics->mutable_address();
  address_specifics->set_id(id);
  return syncer::SyncData::CreateLocalData(id, id, specifics);
}

class TestAutofillTable : public AutofillTable {
 public:
  explicit TestAutofillTable(std::vector<CreditCard> cards_on_disk)
      : cards_on_disk_(cards_on_disk) {}

  ~TestAutofillTable() override {}

  bool GetServerCreditCards(
      std::vector<std::unique_ptr<CreditCard>>* cards) const override {
    for (const auto& card_on_disk : cards_on_disk_)
      cards->push_back(std::make_unique<CreditCard>(card_on_disk));
    return true;
  }

 private:
  std::vector<CreditCard> cards_on_disk_;

  DISALLOW_COPY_AND_ASSIGN(TestAutofillTable);
};

}  // anonymous namespace

// Verify that the link between a card and its billing address from sync is
// present in the generated Autofill objects.
TEST(AutofillWalletSyncableServiceTest,
     PopulateWalletCardsAndAddresses_BillingAddressIdTransfer) {
  std::vector<CreditCard> wallet_cards;
  std::vector<AutofillProfile> wallet_addresses;
  syncer::SyncDataList data_list;

  // Create a Sync data for a card and its billing address.
  data_list.push_back(CreateSyncDataForWalletAddress("1" /* id */));
  data_list.push_back(CreateSyncDataForWalletCreditCard(
      "card1" /* id */, "1" /* billing_address_id */));

  AutofillWalletSyncableService::PopulateWalletCardsAndAddresses(
      data_list, &wallet_cards, &wallet_addresses);

  ASSERT_EQ(1U, wallet_cards.size());
  ASSERT_EQ(1U, wallet_addresses.size());

  // Make sure the card's billing address id is equal to the address' server id.
  EXPECT_EQ(wallet_addresses.back().server_id(),
            wallet_cards.back().billing_address_id());
}

// Verify that the billing address id from the card saved on disk is kept if it
// is a local profile guid.
TEST(AutofillWalletSyncableServiceTest,
     CopyRelevantMetadataFromDisk_KeepLocalAddresses) {
  std::vector<CreditCard> cards_on_disk;
  std::vector<CreditCard> wallet_cards;

  // Create a local profile to be used as a billing address.
  AutofillProfile billing_address;

  // Create a card on disk that refers to that local profile as its billing
  // address.
  cards_on_disk.push_back(CreditCard());
  cards_on_disk.back().set_billing_address_id(billing_address.guid());

  // Create a card pulled from wallet with the same id, but a different billing
  // address id.
  wallet_cards.push_back(CreditCard(cards_on_disk.back()));
  wallet_cards.back().set_billing_address_id("1234");

  // Setup the TestAutofillTable with the cards_on_disk.
  TestAutofillTable table(cards_on_disk);

  AutofillWalletSyncableService::CopyRelevantMetadataFromDisk(table,
                                                              &wallet_cards);

  ASSERT_EQ(1U, wallet_cards.size());

  // Make sure the wallet card replace its billind address id for the one that
  // was saved on disk.
  EXPECT_EQ(cards_on_disk.back().billing_address_id(),
            wallet_cards.back().billing_address_id());
}

// Verify that the billing address id from the card saved on disk is overwritten
// if it does not refer to a local profile.
TEST(AutofillWalletSyncableServiceTest,
     CopyRelevantMetadataFromDisk_OverwriteOtherAddresses) {
  std::string old_billing_id = "1234";
  std::string new_billing_id = "9876";
  std::vector<CreditCard> cards_on_disk;
  std::vector<CreditCard> wallet_cards;

  // Create a card on disk that does not refer to a local profile (which have 36
  // chars ids).
  cards_on_disk.push_back(CreditCard());
  cards_on_disk.back().set_billing_address_id(old_billing_id);

  // Create a card pulled from wallet with the same id, but a different billing
  // address id.
  wallet_cards.push_back(CreditCard(cards_on_disk.back()));
  wallet_cards.back().set_billing_address_id(new_billing_id);

  // Setup the TestAutofillTable with the cards_on_disk.
  TestAutofillTable table(cards_on_disk);

  AutofillWalletSyncableService::CopyRelevantMetadataFromDisk(table,
                                                              &wallet_cards);

  ASSERT_EQ(1U, wallet_cards.size());

  // Make sure the local address billing id that was saved on disk did not
  // replace the new one.
  EXPECT_EQ(new_billing_id, wallet_cards.back().billing_address_id());
}

// Verify that the use stats on disk are kept when server cards are synced.
TEST(AutofillWalletSyncableServiceTest,
     CopyRelevantMetadataFromDisk_KeepUseStats) {
  TestAutofillClock test_clock;
  base::Time arbitrary_time = base::Time::FromDoubleT(25);
  base::Time disk_time = base::Time::FromDoubleT(10);
  test_clock.SetNow(arbitrary_time);

  std::vector<CreditCard> cards_on_disk;
  std::vector<CreditCard> wallet_cards;

  // Create a card on disk with specific use stats.
  cards_on_disk.push_back(CreditCard());
  cards_on_disk.back().set_use_count(3U);
  cards_on_disk.back().set_use_date(disk_time);

  // Create a card pulled from wallet with the same id, but a different billing
  // address id.
  wallet_cards.push_back(CreditCard());
  wallet_cards.back().set_use_count(10U);

  // Setup the TestAutofillTable with the cards_on_disk.
  TestAutofillTable table(cards_on_disk);

  AutofillWalletSyncableService::CopyRelevantMetadataFromDisk(table,
                                                              &wallet_cards);

  ASSERT_EQ(1U, wallet_cards.size());

  // Make sure the use stats from disk were kept
  EXPECT_EQ(3U, wallet_cards.back().use_count());
  EXPECT_EQ(disk_time, wallet_cards.back().use_date());
}

// Verify that the use stats of a new Wallet card are as expected.
TEST(AutofillWalletSyncableServiceTest, NewWalletCard) {
  TestAutofillClock test_clock;
  base::Time arbitrary_time = base::Time::FromDoubleT(25);
  test_clock.SetNow(arbitrary_time);

  std::vector<AutofillProfile> wallet_addresses;
  std::vector<CreditCard> wallet_cards;
  syncer::SyncDataList data_list;

  // Create a Sync data for a card and its billing address.
  data_list.push_back(CreateSyncDataForWalletAddress("1" /* id */));
  data_list.push_back(CreateSyncDataForWalletCreditCard(
      "card1" /* id */, "1" /* billing_address_id */));

  AutofillWalletSyncableService::PopulateWalletCardsAndAddresses(
      data_list, &wallet_cards, &wallet_addresses);

  ASSERT_EQ(1U, wallet_cards.size());

  // The use_count should be 1 and the use_date should be the current time.
  EXPECT_EQ(1U, wallet_cards.back().use_count());
  EXPECT_EQ(arbitrary_time, wallet_cards.back().use_date());
}

// Verify that name on card can be empty.
TEST(AutofillWalletSyncableServiceTest, EmptyNameOnCard) {
  std::vector<CreditCard> wallet_cards;
  std::vector<AutofillProfile> wallet_addresses;
  syncer::SyncDataList data_list;

  // Create a Sync data for a card and its billing address.
  data_list.push_back(CreateSyncDataForWalletCreditCard(
      "card1" /* id */, "1" /* billing_address_id */));

  AutofillWalletSyncableService::PopulateWalletCardsAndAddresses(
      data_list, &wallet_cards, &wallet_addresses);

  ASSERT_EQ(1U, wallet_cards.size());

  // Make sure card holder name can be empty.
  EXPECT_TRUE(
      wallet_cards.back().GetRawInfo(autofill::CREDIT_CARD_NAME_FULL).empty());
  EXPECT_TRUE(
      wallet_cards.back().GetRawInfo(autofill::CREDIT_CARD_NAME_FIRST).empty());
  EXPECT_TRUE(
      wallet_cards.back().GetRawInfo(autofill::CREDIT_CARD_NAME_LAST).empty());
}

TEST(AutofillWalletSyncableServiceTest, ComputeCardsDiff) {
  // Some arbitrary cards for testing.
  CreditCard card0(CreditCard::MASKED_SERVER_CARD, "a");
  CreditCard card1(CreditCard::MASKED_SERVER_CARD, "b");
  CreditCard card2(CreditCard::MASKED_SERVER_CARD, "c");
  CreditCard card3(CreditCard::MASKED_SERVER_CARD, "d");
  // Make sure that card0 < card1 < card2 < card3.
  ASSERT_LT(card0.Compare(card1), 0);
  ASSERT_LT(card1.Compare(card2), 0);
  ASSERT_LT(card2.Compare(card3), 0);

  struct TestCase {
    const char* name;
    const std::vector<CreditCard> old_cards;
    const std::vector<CreditCard> new_cards;
    int expected_added;
    int expected_removed;
  } test_cases[] = {
      {"Both empty", {}, {}, 0, 0},
      {"Old empty", {}, {card0}, 1, 0},
      {"New empty", {card0}, {}, 0, 1},
      {"Identical", {card0, card1}, {card0, card1}, 0, 0},
      {"Identical unsorted", {card0, card1}, {card1, card0}, 0, 0},
      {"Added one", {card0}, {card0, card1}, 1, 0},
      {"Added two", {card1}, {card0, card1, card2}, 2, 0},
      {"Removed one", {card0, card1}, {card1}, 0, 1},
      {"Removed two", {card0, card1, card2}, {card1}, 0, 2},
      {"Replaced one", {card0}, {card1}, 1, 1},
      {"Replaced two", {card0, card1}, {card2, card3}, 2, 2},
      {"Added and removed one", {card0, card1}, {card1, card2}, 1, 1},
  };

  for (const TestCase& test_case : test_cases) {
    SCOPED_TRACE(test_case.name);

    // ComputeDiff expects a vector of unique_ptrs for the old data.
    std::vector<std::unique_ptr<CreditCard>> old_cards_ptrs;
    for (const CreditCard& card : test_case.old_cards)
      old_cards_ptrs.push_back(std::make_unique<CreditCard>(card));

    AutofillWalletSyncableService::Diff diff =
        AutofillWalletSyncableService::ComputeDiff(old_cards_ptrs,
                                                   test_case.new_cards);

    EXPECT_EQ(test_case.expected_added, diff.items_added);
    EXPECT_EQ(test_case.expected_removed, diff.items_removed);
  }
}

TEST(AutofillWalletSyncableServiceTest, ComputeAddressesDiff) {
  // Some arbitrary addresses for testing.
  AutofillProfile address0(AutofillProfile::SERVER_PROFILE, "a");
  AutofillProfile address1(AutofillProfile::SERVER_PROFILE, "b");
  AutofillProfile address2(AutofillProfile::SERVER_PROFILE, "c");
  AutofillProfile address3(AutofillProfile::SERVER_PROFILE, "d");
  address0.SetRawInfo(NAME_FULL, base::ASCIIToUTF16("a"));
  address1.SetRawInfo(NAME_FULL, base::ASCIIToUTF16("b"));
  address2.SetRawInfo(NAME_FULL, base::ASCIIToUTF16("c"));
  address3.SetRawInfo(NAME_FULL, base::ASCIIToUTF16("d"));
  // Make sure that address0 < address1 < address2 < address3.
  ASSERT_LT(address0.Compare(address1), 0);
  ASSERT_LT(address1.Compare(address2), 0);
  ASSERT_LT(address2.Compare(address3), 0);

  struct TestCase {
    const char* name;
    const std::vector<AutofillProfile> old_addresses;
    const std::vector<AutofillProfile> new_addresses;
    int expected_added;
    int expected_removed;
  } test_cases[] = {
      {"Both empty", {}, {}, 0, 0},
      {"Old empty", {}, {address0}, 1, 0},
      {"New empty", {address0}, {}, 0, 1},
      {"Identical", {address0, address1}, {address0, address1}, 0, 0},
      {"Identical unsorted", {address0, address1}, {address1, address0}, 0, 0},
      {"Added one", {address0}, {address0, address1}, 1, 0},
      {"Added two", {address1}, {address0, address1, address2}, 2, 0},
      {"Removed one", {address0, address1}, {address1}, 0, 1},
      {"Removed two", {address0, address1, address2}, {address1}, 0, 2},
      {"Replaced one", {address0}, {address1}, 1, 1},
      {"Replaced two", {address0, address1}, {address2, address3}, 2, 2},
      {"Added and removed one",
       {address0, address1},
       {address1, address2},
       1,
       1},
  };

  for (const TestCase& test_case : test_cases) {
    SCOPED_TRACE(test_case.name);

    // ComputeDiff expects a vector of unique_ptrs for the old data.
    std::vector<std::unique_ptr<AutofillProfile>> old_addresses_ptrs;
    for (const AutofillProfile& address : test_case.old_addresses)
      old_addresses_ptrs.push_back(std::make_unique<AutofillProfile>(address));

    AutofillWalletSyncableService::Diff diff =
        AutofillWalletSyncableService::ComputeDiff(old_addresses_ptrs,
                                                   test_case.new_addresses);

    EXPECT_EQ(test_case.expected_added, diff.items_added);
    EXPECT_EQ(test_case.expected_removed, diff.items_removed);
  }
}

}  // namespace autofill
