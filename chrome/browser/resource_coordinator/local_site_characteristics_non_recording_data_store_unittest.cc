// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_non_recording_data_store.h"

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_store.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

using LocalSiteCharacteristicsNonRecordingDataStoreTest = ::testing::Test;

TEST_F(LocalSiteCharacteristicsNonRecordingDataStoreTest, EndToEnd) {
  content::TestBrowserThreadBundle test_browser_thread_bundle;
  TestingProfile profile;
  const char kTestOrigin[] = "http://www.foo.com";
  std::unique_ptr<LocalSiteCharacteristicsDataStore> recording_data_store =
      std::make_unique<LocalSiteCharacteristicsDataStore>(&profile);
  LocalSiteCharacteristicsNonRecordingDataStore non_recording_data_store(
      recording_data_store.get());

  test_browser_thread_bundle.RunUntilIdle();

  // Ensures that the observation made via a writer created by the non
  // recording data store aren't recorded.

  auto reader = non_recording_data_store.GetReaderForOrigin(kTestOrigin);
  EXPECT_TRUE(reader);
  auto fake_writer = non_recording_data_store.GetWriterForOrigin(kTestOrigin);
  EXPECT_TRUE(fake_writer);
  auto real_writer = recording_data_store->GetWriterForOrigin(kTestOrigin);
  EXPECT_TRUE(real_writer);

  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            reader->UpdatesTitleInBackground());
  fake_writer->NotifySiteLoaded();
  fake_writer->NotifyUpdatesTitleInBackground();
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            reader->UpdatesTitleInBackground());

  real_writer->NotifySiteLoaded();
  real_writer->NotifyUpdatesTitleInBackground();
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            reader->UpdatesTitleInBackground());

  // These unload events shouldn't be registered, make sure that they aren't by
  // unloading the site more time than it has been loaded.
  fake_writer->NotifySiteUnloaded();
  fake_writer->NotifySiteUnloaded();

  real_writer->NotifySiteUnloaded();

  real_writer.reset();
  reader.reset();

  test_browser_thread_bundle.RunUntilIdle();

  recording_data_store.reset();

  // Wait for the database of the recording store to be flushed to disk before
  // terminating this test, otherwise the profile might get deleted while the
  // database is still being flushed to disk and this could prevent from
  // deleting its temporary directory.
  test_browser_thread_bundle.RunUntilIdle();
}

}  // namespace resource_coordinator
