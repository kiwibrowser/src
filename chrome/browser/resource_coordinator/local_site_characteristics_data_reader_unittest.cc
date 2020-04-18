// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_reader.h"

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/test/simple_test_tick_clock.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_data_impl.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_data_unittest_utils.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

class LocalSiteCharacteristicsDataReaderTest : public ::testing::Test {
 protected:
  // The constructors needs to call 'new' directly rather than using the
  // base::MakeRefCounted helper function because the constructor of
  // LocalSiteCharacteristicsDataImpl is protected and not visible to
  // base::MakeRefCounted.
  LocalSiteCharacteristicsDataReaderTest()
      : scoped_set_tick_clock_for_testing_(&test_clock_) {
    test_impl_ =
        base::WrapRefCounted(new internal::LocalSiteCharacteristicsDataImpl(
            "foo.com", &delegate_, &database_));
    test_impl_->NotifySiteLoaded();
    LocalSiteCharacteristicsDataReader* reader =
        new LocalSiteCharacteristicsDataReader(test_impl_.get());
    reader_ = base::WrapUnique(reader);
  }

  ~LocalSiteCharacteristicsDataReaderTest() override {
    test_impl_->NotifySiteUnloaded();
  }

  base::SimpleTestTickClock test_clock_;
  ScopedSetTickClockForTesting scoped_set_tick_clock_for_testing_;

  // The mock delegate used by the LocalSiteCharacteristicsDataImpl objects
  // created by this class, NiceMock is used to avoid having to set expectations
  // in test cases that don't care about this.
  ::testing::NiceMock<
      testing::MockLocalSiteCharacteristicsDataImplOnDestroyDelegate>
      delegate_;

  // The LocalSiteCharacteristicsDataImpl object used in these tests.
  scoped_refptr<internal::LocalSiteCharacteristicsDataImpl> test_impl_;

  // A LocalSiteCharacteristicsDataReader object associated with the origin used
  // to create this object.
  std::unique_ptr<LocalSiteCharacteristicsDataReader> reader_;

  testing::NoopLocalSiteCharacteristicsDatabase database_;

  DISALLOW_COPY_AND_ASSIGN(LocalSiteCharacteristicsDataReaderTest);
};

TEST_F(LocalSiteCharacteristicsDataReaderTest, TestAccessors) {
  // Initially we have no information about any of the features.
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            reader_->UpdatesFaviconInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            reader_->UpdatesTitleInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            reader_->UsesAudioInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            reader_->UsesNotificationsInBackground());

  // Simulates a title update event, make sure it gets reported directly.
  test_impl_->NotifyUpdatesTitleInBackground();

  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            reader_->UpdatesTitleInBackground());

  // Advance the clock by a large amount of time, enough for the unused features
  // observation windows to expire.
  test_clock_.Advance(base::TimeDelta::FromDays(31));

  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureNotInUse,
            reader_->UpdatesFaviconInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            reader_->UpdatesTitleInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureNotInUse,
            reader_->UsesAudioInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureNotInUse,
            reader_->UsesNotificationsInBackground());
}

}  // namespace resource_coordinator
