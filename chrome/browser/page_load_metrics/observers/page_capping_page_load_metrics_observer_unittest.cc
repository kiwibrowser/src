// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/page_capping_page_load_metrics_observer.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/data_use_measurement/page_load_capping/chrome_page_load_capping_features.h"
#include "chrome/browser/data_use_measurement/page_load_capping/page_load_capping_infobar_delegate.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/infobars/mock_infobar_service.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_tracker.h"
#include "url/gurl.h"

namespace {
const char kTestURL[] = "http://www.test.com";
}

class PageCappingObserverTest
    : public page_load_metrics::PageLoadMetricsObserverTestHarness {
 public:
  PageCappingObserverTest() = default;
  ~PageCappingObserverTest() override = default;

  void SetUpTest() {
    MockInfoBarService::CreateForWebContents(web_contents());
    NavigateAndCommit(GURL(kTestURL));
  }

  size_t InfoBarCount() { return infobar_service()->infobar_count(); }

  void RemoveAllInfoBars() { infobar_service()->RemoveAllInfoBars(false); }

  InfoBarService* infobar_service() {
    return InfoBarService::FromWebContents(web_contents());
  }

 protected:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker) override {
    tracker->AddObserver(
        std::make_unique<PageCappingPageLoadMetricsObserver>());
  }
};

TEST_F(PageCappingObserverTest, ExperimentDisabled) {
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndDisableFeature(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages);
  SetUpTest();

  // A resource slightly over 1 MB.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (1 * 1024 * 1024) + 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // The infobar should not show even though the cap would be met because the
  // feature is disabled.
  SimulateLoadedResource(resource);

  EXPECT_EQ(0u, InfoBarCount());
}

TEST_F(PageCappingObserverTest, DefaultThresholdNotMetNonMedia) {
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages,
      {});
  SetUpTest();

  // A resource slightly under 5 MB, the default page capping threshold.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (5 * 1024 * 1024) - 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // The cap is not met, so the infobar should not show.
  SimulateLoadedResource(resource);

  EXPECT_EQ(0u, InfoBarCount());
}

TEST_F(PageCappingObserverTest, DefaultThresholdMetNonMedia) {
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages,
      {});
  SetUpTest();

  // A resource slightly over 5 MB, the default page capping threshold.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (5 * 1024 * 1024) + 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // The cap is not met, so the infobar should not show.
  SimulateLoadedResource(resource);

  EXPECT_EQ(1u, InfoBarCount());
}

TEST_F(PageCappingObserverTest, DefaultThresholdNotMetMedia) {
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages,
      {});
  SetUpTest();

  SimulateMediaPlayed();

  // A resource slightly under 15 MB, the default media page capping threshold.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (15 * 1024 * 1024) - 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // The cap is not met, so the infobar should not show.
  SimulateLoadedResource(resource);

  EXPECT_EQ(0u, InfoBarCount());
}

TEST_F(PageCappingObserverTest, DefaultThresholdMetMedia) {
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages,
      {});
  SetUpTest();

  SimulateMediaPlayed();

  // A resource slightly over 15 MB, the default media page capping threshold.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (15 * 1024 * 1024) + 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // The cap is not met, so the infobar should not show.
  SimulateLoadedResource(resource);

  EXPECT_EQ(1u, InfoBarCount());
}

TEST_F(PageCappingObserverTest, NotEnoughForThreshold) {
  std::map<std::string, std::string> feature_parameters = {
      {"MediaPageCapMB", "1"}, {"PageCapMB", "1"}};
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages,
      feature_parameters);
  SetUpTest();

  // A resource slightly under 1 MB.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (1 * 1024 * 1024) - 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // The cap is not met, so the infobar should not show.
  SimulateLoadedResource(resource);

  EXPECT_EQ(0u, InfoBarCount());
}

TEST_F(PageCappingObserverTest, InfobarOnlyShownOnce) {
  std::map<std::string, std::string> feature_parameters = {
      {"MediaPageCapMB", "1"}, {"PageCapMB", "1"}};
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages,
      feature_parameters);
  SetUpTest();

  // A resource slightly over 1 MB.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (1 * 1024 * 1024) + 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // This should trigger the infobar.
  SimulateLoadedResource(resource);
  EXPECT_EQ(1u, InfoBarCount());
  // The infobar is already being shown, so this should not trigger and infobar.
  SimulateLoadedResource(resource);
  EXPECT_EQ(1u, InfoBarCount());

  // Clear all infobars.
  RemoveAllInfoBars();
  // Verify the infobars are clear.
  EXPECT_EQ(0u, InfoBarCount());
  // This would trigger and infobar if one was not already shown from this
  // observer.
  SimulateLoadedResource(resource);
  EXPECT_EQ(0u, InfoBarCount());
}

TEST_F(PageCappingObserverTest, MediaCap) {
  std::map<std::string, std::string> feature_parameters = {
      {"MediaPageCapMB", "10"}, {"PageCapMB", "1"}};
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages,
      feature_parameters);
  SetUpTest();

  // Show that media has played.
  SimulateMediaPlayed();

  // A resource slightly under 10 MB.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (10 * 1024 * 1024) - 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // This should not trigger an infobar as the media cap is not met.
  SimulateLoadedResource(resource);
  EXPECT_EQ(0u, InfoBarCount());
  // Adding more data should now trigger the infobar.
  SimulateLoadedResource(resource);
  EXPECT_EQ(1u, InfoBarCount());
}

TEST_F(PageCappingObserverTest, PageCap) {
  std::map<std::string, std::string> feature_parameters = {
      {"MediaPageCapMB", "1"}, {"PageCapMB", "10"}};
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      data_use_measurement::page_load_capping::features::kDetectingHeavyPages,
      feature_parameters);
  SetUpTest();

  // A resource slightly under 10 MB.
  page_load_metrics::ExtraRequestCompleteInfo resource = {
      GURL(kTestURL),
      net::HostPortPair(),
      -1 /* frame_tree_node_id */,
      false /* was_cached */,
      (10 * 1024 * 1024) - 10 /* raw_body_bytes */,
      0 /* original_network_content_length */,
      nullptr,
      content::ResourceType::RESOURCE_TYPE_SCRIPT,
      0,
      {} /* load_timing_info */};

  // This should not trigger an infobar as the non-media cap is not met.
  SimulateLoadedResource(resource);
  EXPECT_EQ(0u, InfoBarCount());
  // Adding more data should now trigger the infobar.
  SimulateLoadedResource(resource);
  EXPECT_EQ(1u, InfoBarCount());
}
