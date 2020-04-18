// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_server_urls.h"

#include "components/offline_pages/core/offline_page_feature.h"
#include "components/variations/variations_params_manager.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace offline_pages {

namespace {
const char kTestOfflinePagesSuggestionsServerEndpoint[] =
    "https://test-offlinepages-pa.sandbox.googleapis.com/";
const char kInvalidServerEndpoint[] = "^__^";
const char kInvalidSchemeServerEndpoint[] =
    "http://test-offlinepages-pa.sandbox.googleapis.com/";
}  // namespace

class PrefetchServerURLsTest : public testing::Test {
 public:
  PrefetchServerURLsTest() = default;
  ~PrefetchServerURLsTest() override = default;
  void SetTestingServerEndpoint(const std::string& server_config);

 private:
  variations::testing::VariationParamsManager params_manager_;
};

void PrefetchServerURLsTest::SetTestingServerEndpoint(
    const std::string& server_config) {
  params_manager_.ClearAllVariationParams();
  params_manager_.SetVariationParamsWithFeatureAssociations(
      kPrefetchingOfflinePagesFeature.name,
      {{"offline_pages_backend", server_config}},
      {kPrefetchingOfflinePagesFeature.name});
}

TEST_F(PrefetchServerURLsTest, TestVariationsConfig) {
  GURL default_server(kPrefetchServer);
  GURL request_url =
      GeneratePageBundleRequestURL(version_info::Channel::UNKNOWN);
  EXPECT_EQ(default_server.host(), request_url.host());
  EXPECT_TRUE(request_url.SchemeIsCryptographic());

  // Test reset to a valid, HTTPS URL.
  SetTestingServerEndpoint(kTestOfflinePagesSuggestionsServerEndpoint);
  request_url = GeneratePageBundleRequestURL(version_info::Channel::UNKNOWN);
  EXPECT_EQ("test-offlinepages-pa.sandbox.googleapis.com", request_url.host());
  EXPECT_TRUE(request_url.SchemeIsCryptographic());

  // Test other variations of invalid URLS.
  // First, a completely bogus endpoint.
  SetTestingServerEndpoint(kInvalidServerEndpoint);
  request_url = GeneratePageBundleRequestURL(version_info::Channel::UNKNOWN);
  EXPECT_EQ(default_server.host(), request_url.host());
  EXPECT_TRUE(request_url.SchemeIsCryptographic());

  // Then a valid URL with a non-cryptographic scheme.
  SetTestingServerEndpoint(kInvalidSchemeServerEndpoint);
  request_url = GeneratePageBundleRequestURL(version_info::Channel::UNKNOWN);
  EXPECT_EQ(default_server.host(), request_url.host());
  EXPECT_TRUE(request_url.SchemeIsCryptographic());
}

}  // namespace offline_pages
