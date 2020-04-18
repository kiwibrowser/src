// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_feature.h"

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

TEST(OfflinePageFeatureTest, OfflineBookmarks) {
  // Enabled by default.
  EXPECT_TRUE(offline_pages::IsOfflineBookmarksEnabled());

  // Check if helper method works correctly when the features is disabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kOfflineBookmarksFeature);
  EXPECT_FALSE(offline_pages::IsOfflineBookmarksEnabled());
}

TEST(OfflinePageFeatureTest, OffliningRecentPages) {
  // Enabled by default.
  EXPECT_TRUE(offline_pages::IsOffliningRecentPagesEnabled());

  // Check if helper method works correctly when the features is disabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kOffliningRecentPagesFeature);
  EXPECT_FALSE(offline_pages::IsOffliningRecentPagesEnabled());
}

TEST(OfflinePageFeatureTest, OfflinePagesSharing) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::IsOfflinePagesSharingEnabled());

  // Check if helper method works correctly when the features is disabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kOfflinePagesSharingFeature);
  EXPECT_TRUE(offline_pages::IsOfflinePagesSharingEnabled());
}

TEST(OfflinePageFeatureTest, OfflinePagesSvelteConcurrentLoading) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::IsOfflinePagesSvelteConcurrentLoadingEnabled());

  // Check if helper method works correctly when the features is enabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      kOfflinePagesSvelteConcurrentLoadingFeature);
  EXPECT_TRUE(offline_pages::IsOfflinePagesSvelteConcurrentLoadingEnabled());
}

TEST(OfflinePageFeatureTest, OfflinePagesLoadSignalCollecting) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::IsOfflinePagesLoadSignalCollectingEnabled());

  // Check if helper method works correctly when the features is enabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      kOfflinePagesLoadSignalCollectingFeature);
  EXPECT_TRUE(offline_pages::IsOfflinePagesLoadSignalCollectingEnabled());
}

TEST(OfflinePageFeatureTest, OfflinePagesPrefetching) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::IsPrefetchingOfflinePagesEnabled());

  // Check if helper method works correctly when the features is enabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kPrefetchingOfflinePagesFeature);
  EXPECT_TRUE(offline_pages::IsPrefetchingOfflinePagesEnabled());
}

TEST(OfflinePageFeatureTest, OfflinePagesPrefetchingUI) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::IsOfflinePagesPrefetchingUIEnabled());

  // This feature is enabled by default but depends on the core prefetching
  // feature.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kPrefetchingOfflinePagesFeature);
  EXPECT_TRUE(offline_pages::IsOfflinePagesPrefetchingUIEnabled());
}

TEST(OfflinePageFeatureTest, OfflinePagesLimitlessPrefetching) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::IsLimitlessPrefetchingEnabled());

  // This feature depends on the core prefetching feature.
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeature(kPrefetchingOfflinePagesFeature);
    EXPECT_FALSE(offline_pages::IsLimitlessPrefetchingEnabled());
  }
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeature(
        kOfflinePagesLimitlessPrefetchingFeature);
    EXPECT_FALSE(offline_pages::IsLimitlessPrefetchingEnabled());
  }

  // Check if helper method works correctly when all required features are
  // enabled.
  // TODO(https://crbug.com/803584): fix limitless mode or fully remove it.
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitWithFeatures(
        {kPrefetchingOfflinePagesFeature,
         kOfflinePagesLimitlessPrefetchingFeature},
        {});
    EXPECT_FALSE(offline_pages::IsLimitlessPrefetchingEnabled());
  }
}

TEST(OfflinePageFeatureTest, OfflinePagesInDownloadHomeOpenInCct) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::ShouldOfflinePagesInDownloadHomeOpenInCct());

  // Check if helper method works correctly when the features is enabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      kOfflinePagesInDownloadHomeOpenInCctFeature);
  EXPECT_TRUE(offline_pages::ShouldOfflinePagesInDownloadHomeOpenInCct());
}

TEST(OfflinePageFeatureTest, OfflinePagesDescriptiveFailStatus) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::IsOfflinePagesDescriptiveFailStatusEnabled());

  // Check if helper method works correctly when the features is enabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      kOfflinePagesDescriptiveFailStatusFeature);
  EXPECT_TRUE(offline_pages::IsOfflinePagesDescriptiveFailStatusEnabled());
}

TEST(OfflinePageFeatureTest, OfflinePagesDescriptivePendingStatus) {
  // Enabled by default.
  EXPECT_TRUE(offline_pages::IsOfflinePagesDescriptivePendingStatusEnabled());

  // Check if helper method works correctly when the features is enabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kOfflinePagesDescriptivePendingStatusFeature);
  EXPECT_FALSE(offline_pages::IsOfflinePagesDescriptivePendingStatusEnabled());
}

TEST(OfflinePageFeatureTest, AlternateDinoPage) {
  // Disabled by default.
  EXPECT_FALSE(offline_pages::ShouldShowAlternateDinoPage());

  // Check if helper method works correctly when the features is enabled.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      kOfflinePagesShowAlternateDinoPageFeature);
  EXPECT_TRUE(offline_pages::ShouldShowAlternateDinoPage());
}

}  // namespace offline_pages
