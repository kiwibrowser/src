// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/core/serialized_navigation_entry.h"

#include <stdint.h>

#include <cstddef>
#include <memory>
#include <string>

#include "base/pickle.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/sessions/core/serialized_navigation_driver.h"
#include "components/sessions/core/serialized_navigation_entry_test_helper.h"
#include "components/sync/base/time.h"
#include "components/sync/protocol/session_specifics.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace sessions {
namespace {

// Create a sync_pb::TabNavigation from the constants above.
sync_pb::TabNavigation MakeSyncDataForTest() {
  sync_pb::TabNavigation sync_data;
  sync_data.set_virtual_url(test_data::kVirtualURL.spec());
  sync_data.set_referrer(test_data::kReferrerURL.spec());
  sync_data.set_obsolete_referrer_policy(test_data::kReferrerPolicy);
  sync_data.set_correct_referrer_policy(test_data::kReferrerPolicy);
  sync_data.set_title(base::UTF16ToUTF8(test_data::kTitle));
  sync_data.set_page_transition(
      sync_pb::SyncEnums_PageTransition_AUTO_SUBFRAME);
  sync_data.set_unique_id(test_data::kUniqueID);
  sync_data.set_timestamp_msec(syncer::TimeToProtoTime(test_data::kTimestamp));
  sync_data.set_redirect_type(sync_pb::SyncEnums::CLIENT_REDIRECT);
  sync_data.set_navigation_home_page(true);
  sync_data.set_favicon_url(test_data::kFaviconURL.spec());
  sync_data.set_http_status_code(test_data::kHttpStatusCode);
  // The redirect chain only syncs one way.
  return sync_data;
}

// Create a default SerializedNavigationEntry.  All its fields should be
// initialized to their respective default values.
TEST(SerializedNavigationEntryTest, DefaultInitializer) {
  const SerializedNavigationEntry navigation;
  EXPECT_EQ(-1, navigation.index());
  EXPECT_EQ(0, navigation.unique_id());
  EXPECT_EQ(GURL(), navigation.referrer_url());
  EXPECT_EQ(
      SerializedNavigationDriver::Get()->GetDefaultReferrerPolicy(),
      navigation.referrer_policy());
  EXPECT_EQ(GURL(), navigation.virtual_url());
  EXPECT_TRUE(navigation.title().empty());
  EXPECT_EQ(std::string(), navigation.encoded_page_state());
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      navigation.transition_type(), ui::PAGE_TRANSITION_TYPED));
  EXPECT_FALSE(navigation.has_post_data());
  EXPECT_EQ(-1, navigation.post_id());
  EXPECT_EQ(GURL(), navigation.original_request_url());
  EXPECT_FALSE(navigation.is_overriding_user_agent());
  EXPECT_TRUE(navigation.timestamp().is_null());
  EXPECT_FALSE(navigation.favicon_url().is_valid());
  EXPECT_EQ(0, navigation.http_status_code());
  EXPECT_EQ(0U, navigation.redirect_chain().size());
}

// Create a SerializedNavigationEntry from a sync_pb::TabNavigation.  All its
// fields should match the protocol buffer's if it exists there, and
// sbould be set to the default value otherwise.
TEST(SerializedNavigationEntryTest, FromSyncData) {
  const sync_pb::TabNavigation sync_data = MakeSyncDataForTest();

  const SerializedNavigationEntry& navigation =
      SerializedNavigationEntry::FromSyncData(test_data::kIndex, sync_data);

  EXPECT_EQ(test_data::kIndex, navigation.index());
  EXPECT_EQ(test_data::kUniqueID, navigation.unique_id());
  EXPECT_EQ(test_data::kReferrerURL, navigation.referrer_url());
  EXPECT_EQ(test_data::kReferrerPolicy, navigation.referrer_policy());
  EXPECT_EQ(test_data::kVirtualURL, navigation.virtual_url());
  EXPECT_EQ(test_data::kTitle, navigation.title());
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      navigation.transition_type(), test_data::kTransitionType));
  EXPECT_FALSE(navigation.has_post_data());
  EXPECT_EQ(-1, navigation.post_id());
  EXPECT_EQ(GURL(), navigation.original_request_url());
  EXPECT_FALSE(navigation.is_overriding_user_agent());
  EXPECT_EQ(test_data::kTimestamp, navigation.timestamp());
  EXPECT_EQ(test_data::kFaviconURL, navigation.favicon_url());
  EXPECT_EQ(test_data::kHttpStatusCode, navigation.http_status_code());
  // The redirect chain only syncs one way.
}

// Create a SerializedNavigationEntry, pickle it, then create another one by
// unpickling.  The new one should match the old one except for fields
// that aren't pickled, which should be set to default values.
TEST(SerializedNavigationEntryTest, Pickle) {
  const SerializedNavigationEntry old_navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();

  base::Pickle pickle;
  old_navigation.WriteToPickle(30000, &pickle);

  SerializedNavigationEntry new_navigation;
  base::PickleIterator pickle_iterator(pickle);
  EXPECT_TRUE(new_navigation.ReadFromPickle(&pickle_iterator));

  // Fields that are written to the pickle.
  EXPECT_EQ(test_data::kIndex, new_navigation.index());
  EXPECT_EQ(test_data::kReferrerURL, new_navigation.referrer_url());
  EXPECT_EQ(test_data::kReferrerPolicy, new_navigation.referrer_policy());
  EXPECT_EQ(test_data::kVirtualURL, new_navigation.virtual_url());
  EXPECT_EQ(test_data::kTitle, new_navigation.title());
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      new_navigation.transition_type(), test_data::kTransitionType));
  EXPECT_EQ(test_data::kHasPostData, new_navigation.has_post_data());
  EXPECT_EQ(test_data::kOriginalRequestURL,
            new_navigation.original_request_url());
  EXPECT_EQ(test_data::kIsOverridingUserAgent,
            new_navigation.is_overriding_user_agent());
  EXPECT_EQ(test_data::kTimestamp, new_navigation.timestamp());
  EXPECT_EQ(test_data::kHttpStatusCode, new_navigation.http_status_code());

  ASSERT_EQ(2U, new_navigation.extended_info_map().size());
  ASSERT_EQ(1U, new_navigation.extended_info_map().count(
                    test_data::kExtendedInfoKey1));
  EXPECT_EQ(
      test_data::kExtendedInfoValue1,
      new_navigation.extended_info_map().at(test_data::kExtendedInfoKey1));
  ASSERT_EQ(1U, new_navigation.extended_info_map().count(
                    test_data::kExtendedInfoKey2));
  EXPECT_EQ(
      test_data::kExtendedInfoValue2,
      new_navigation.extended_info_map().at(test_data::kExtendedInfoKey2));

  // Fields that are not written to the pickle.
  EXPECT_EQ(0, new_navigation.unique_id());
  EXPECT_EQ(std::string(), new_navigation.encoded_page_state());
  EXPECT_EQ(-1, new_navigation.post_id());
  EXPECT_EQ(0U, new_navigation.redirect_chain().size());
}

// Create a SerializedNavigationEntry, then create a sync protocol buffer from
// it.  The protocol buffer should have matching fields to the
// SerializedNavigationEntry (when applicable).
TEST(SerializedNavigationEntryTest, ToSyncData) {
  const SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();
  const sync_pb::TabNavigation sync_data = navigation.ToSyncData();

  EXPECT_EQ(test_data::kVirtualURL.spec(), sync_data.virtual_url());
  EXPECT_EQ(test_data::kReferrerURL.spec(), sync_data.referrer());
  EXPECT_EQ(test_data::kTitle, base::ASCIIToUTF16(sync_data.title()));
  EXPECT_EQ(sync_pb::SyncEnums_PageTransition_AUTO_SUBFRAME,
            sync_data.page_transition());
  EXPECT_TRUE(sync_data.has_redirect_type());
  EXPECT_EQ(test_data::kUniqueID, sync_data.unique_id());
  EXPECT_EQ(syncer::TimeToProtoTime(test_data::kTimestamp),
            sync_data.timestamp_msec());
  EXPECT_EQ(test_data::kTimestamp.ToInternalValue(), sync_data.global_id());
  EXPECT_EQ(test_data::kFaviconURL.spec(), sync_data.favicon_url());
  EXPECT_EQ(test_data::kHttpStatusCode, sync_data.http_status_code());
  // The proto navigation redirects don't include the final chain entry
  // (because it didn't redirect) so the lengths should differ by 1.
  ASSERT_EQ(3, sync_data.navigation_redirect_size() + 1);
  EXPECT_EQ(test_data::kRedirectURL0.spec(),
            sync_data.navigation_redirect(0).url());
  EXPECT_EQ(test_data::kRedirectURL1.spec(),
            sync_data.navigation_redirect(1).url());
  EXPECT_FALSE(sync_data.has_last_navigation_redirect_url());
  EXPECT_FALSE(sync_data.has_replaced_navigation());
}

// Specifically test the |replaced_navigation| field, which should be populated
// when the navigation entry has been replaced by another entry (e.g.
// history.pushState()).
TEST(SerializedNavigationEntryTest, ReplacedNavigation) {
  const GURL kReplacedURL = GURL("http://replaced-url.com");
  const int kReplacedTimestampMs = 79;
  const ui::PageTransition kReplacedPageTransition =
      ui::PAGE_TRANSITION_AUTO_BOOKMARK;

  SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();
  SerializedNavigationEntryTestHelper::SetReplacedEntryData(
      {kReplacedURL, syncer::ProtoTimeToTime(kReplacedTimestampMs),
       kReplacedPageTransition},
      &navigation);

  const sync_pb::TabNavigation sync_data = navigation.ToSyncData();
  EXPECT_TRUE(sync_data.has_replaced_navigation());
  EXPECT_EQ(kReplacedURL.spec(),
            sync_data.replaced_navigation().first_committed_url());
  EXPECT_EQ(kReplacedTimestampMs,
            sync_data.replaced_navigation().first_timestamp_msec());
  EXPECT_EQ(sync_pb::SyncEnums_PageTransition_AUTO_BOOKMARK,
            sync_data.replaced_navigation().first_page_transition());
}

// Test that the last_navigation_redirect_url is set when needed.  This test is
// just like the above, but with a different virtual_url.  Create a
// SerializedNavigationEntry, then create a sync protocol buffer from it.  The
// protocol buffer should have a last_navigation_redirect_url.
TEST(SerializedNavigationEntryTest, LastNavigationRedirectUrl) {
  SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();
  SerializedNavigationEntryTestHelper::SetVirtualURL(
      test_data::kOtherURL, &navigation);

  const sync_pb::TabNavigation sync_data = navigation.ToSyncData();
  EXPECT_TRUE(sync_data.has_last_navigation_redirect_url());
  EXPECT_EQ(test_data::kVirtualURL.spec(),
            sync_data.last_navigation_redirect_url());

  // The redirect chain should be the same as in the above test.
  ASSERT_EQ(3, sync_data.navigation_redirect_size() + 1);
  EXPECT_EQ(test_data::kRedirectURL0.spec(),
            sync_data.navigation_redirect(0).url());
  EXPECT_EQ(test_data::kRedirectURL1.spec(),
            sync_data.navigation_redirect(1).url());
}

// Ensure all transition types and qualifiers are converted to/from the sync
// SerializedNavigationEntry representation properly.
TEST(SerializedNavigationEntryTest, TransitionTypes) {
  SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();

  for (uint32_t core_type = ui::PAGE_TRANSITION_LINK;
       core_type < ui::PAGE_TRANSITION_LAST_CORE; ++core_type) {
    // Because qualifier is a uint32_t, left shifting will eventually overflow
    // and hit zero again. SERVER_REDIRECT, as the last qualifier and also
    // in place of the sign bit, is therefore the last transition before
    // breaking.
    for (uint32_t qualifier = ui::PAGE_TRANSITION_FORWARD_BACK; qualifier != 0;
         qualifier <<= 1) {
      if (qualifier == 0x08000000)
        continue;  // 0x08000000 is not a valid qualifier.
      ui::PageTransition transition =
          ui::PageTransitionFromInt(core_type | qualifier);
      SerializedNavigationEntryTestHelper::SetTransitionType(
          transition, &navigation);

      const sync_pb::TabNavigation& sync_data = navigation.ToSyncData();
      const SerializedNavigationEntry& constructed_nav =
          SerializedNavigationEntry::FromSyncData(test_data::kIndex, sync_data);
      const ui::PageTransition constructed_transition =
          constructed_nav.transition_type();

      EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
          constructed_transition, transition));
    }
  }
}

}  // namespace
}  // namespace sessions
