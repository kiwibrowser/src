// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/local_site_characteristics_data_writer.h"

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_data_impl.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_data_unittest_utils.h"
#include "chrome/browser/resource_coordinator/local_site_characteristics_feature_usage.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

class LocalSiteCharacteristicsDataWriterTest : public ::testing::Test {
 protected:
  // The constructors needs to call 'new' directly rather than using the
  // base::MakeRefCounted helper function because the constructor of
  // LocalSiteCharacteristicsDataImpl is protected and not visible to
  // base::MakeRefCounted.
  LocalSiteCharacteristicsDataWriterTest()
      : test_impl_(base::WrapRefCounted(
            new internal::LocalSiteCharacteristicsDataImpl("foo.com",
                                                           &delegate_,
                                                           &database_))) {
    LocalSiteCharacteristicsDataWriter* writer =
        new LocalSiteCharacteristicsDataWriter(test_impl_.get());
    writer_ = base::WrapUnique(writer);
  }

  ~LocalSiteCharacteristicsDataWriterTest() override = default;

  bool TabIsLoaded() { return test_impl_->IsLoaded(); }

  // The mock delegate used by the LocalSiteCharacteristicsDataImpl objects
  // created by this class, NiceMock is used to avoid having to set
  // expectations in test cases that don't care about this.
  ::testing::NiceMock<
      testing::MockLocalSiteCharacteristicsDataImplOnDestroyDelegate>
      delegate_;

  testing::NoopLocalSiteCharacteristicsDatabase database_;

  // The LocalSiteCharacteristicsDataImpl object used in these tests.
  scoped_refptr<internal::LocalSiteCharacteristicsDataImpl> test_impl_;

  // A LocalSiteCharacteristicsDataWriter object associated with the origin used
  // to create this object.
  std::unique_ptr<LocalSiteCharacteristicsDataWriter> writer_;

  DISALLOW_COPY_AND_ASSIGN(LocalSiteCharacteristicsDataWriterTest);
};

TEST_F(LocalSiteCharacteristicsDataWriterTest, TestModifiers) {
  // Make sure that we initially have no information about any of the features
  // and that the site is in an unloaded state.
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UpdatesFaviconInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UpdatesTitleInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UsesAudioInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UsesNotificationsInBackground());

  // Test the OnTabLoaded function.
  EXPECT_FALSE(TabIsLoaded());
  writer_->NotifySiteLoaded();
  EXPECT_TRUE(TabIsLoaded());

  // Test all the modifiers.

  writer_->NotifyUpdatesFaviconInBackground();
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UpdatesFaviconInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UpdatesTitleInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UsesAudioInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UsesNotificationsInBackground());

  writer_->NotifyUpdatesTitleInBackground();
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UpdatesFaviconInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UpdatesTitleInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UsesAudioInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UsesNotificationsInBackground());

  writer_->NotifyUsesAudioInBackground();
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UpdatesFaviconInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UpdatesTitleInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UsesAudioInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureUsageUnknown,
            test_impl_->UsesNotificationsInBackground());

  writer_->NotifyUsesNotificationsInBackground();
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UpdatesFaviconInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UpdatesTitleInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UsesAudioInBackground());
  EXPECT_EQ(SiteFeatureUsage::kSiteFeatureInUse,
            test_impl_->UsesNotificationsInBackground());

  writer_->NotifySiteUnloaded();
}

}  // namespace resource_coordinator
