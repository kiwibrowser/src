// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/client_hints_preferences.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class ClientHintsPreferencesTest : public testing::Test {};

TEST_F(ClientHintsPreferencesTest, BasicSecure) {
  struct TestCase {
    const char* header_value;
    bool expectation_resource_width;
    bool expectation_dpr;
    bool expectation_viewport_width;
    bool expectation_rtt;
    bool expectation_downlink;
    bool expectation_ect;
  } cases[] = {
      {"width, dpr, viewportWidth", true, true, false, false, false, false},
      {"WiDtH, dPr, viewport-width, rtt, downlink, ect", true, true, true, true,
       true, true},
      {"WiDtH, dPr, viewport-width, rtt, downlink, effective-connection-type",
       true, true, true, true, true, false},
      {"WIDTH, DPR, VIWEPROT-Width", true, true, false, false, false, false},
      {"VIewporT-Width, wutwut, width", true, false, true, false, false, false},
      {"dprw", false, false, false, false, false, false},
      {"DPRW", false, false, false, false, false, false},
  };

  for (const auto& test_case : cases) {
    ClientHintsPreferences preferences;
    const KURL kurl(String::FromUTF8("https://www.google.com/"));
    preferences.UpdateFromAcceptClientHintsHeader(test_case.header_value, kurl,
                                                  nullptr);
    EXPECT_EQ(
        test_case.expectation_resource_width,
        preferences.ShouldSend(mojom::WebClientHintsType::kResourceWidth));
    EXPECT_EQ(test_case.expectation_dpr,
              preferences.ShouldSend(mojom::WebClientHintsType::kDpr));
    EXPECT_EQ(
        test_case.expectation_viewport_width,
        preferences.ShouldSend(mojom::WebClientHintsType::kViewportWidth));
    EXPECT_EQ(test_case.expectation_rtt,
              preferences.ShouldSend(mojom::WebClientHintsType::kRtt));
    EXPECT_EQ(test_case.expectation_downlink,
              preferences.ShouldSend(mojom::WebClientHintsType::kDownlink));
    EXPECT_EQ(test_case.expectation_ect,
              preferences.ShouldSend(mojom::WebClientHintsType::kEct));

    // Calling UpdateFromAcceptClientHintsHeader with empty header should have
    // no impact on client hint preferences.
    preferences.UpdateFromAcceptClientHintsHeader("", kurl, nullptr);
    EXPECT_EQ(
        test_case.expectation_resource_width,
        preferences.ShouldSend(mojom::WebClientHintsType::kResourceWidth));
    EXPECT_EQ(test_case.expectation_dpr,
              preferences.ShouldSend(mojom::WebClientHintsType::kDpr));
    EXPECT_EQ(
        test_case.expectation_viewport_width,
        preferences.ShouldSend(mojom::WebClientHintsType::kViewportWidth));

    // Calling UpdateFromAcceptClientHintsHeader with an invalid header should
    // have no impact on client hint preferences.
    preferences.UpdateFromAcceptClientHintsHeader("foobar", kurl, nullptr);
    EXPECT_EQ(
        test_case.expectation_resource_width,
        preferences.ShouldSend(mojom::WebClientHintsType::kResourceWidth));
    EXPECT_EQ(test_case.expectation_dpr,
              preferences.ShouldSend(mojom::WebClientHintsType::kDpr));
    EXPECT_EQ(
        test_case.expectation_viewport_width,
        preferences.ShouldSend(mojom::WebClientHintsType::kViewportWidth));
  }
}

TEST_F(ClientHintsPreferencesTest, Insecure) {
  for (const auto& use_secure_url : {false, true}) {
    ClientHintsPreferences preferences;
    const KURL kurl = use_secure_url
                          ? KURL(String::FromUTF8("https://www.google.com/"))
                          : KURL(String::FromUTF8("http://www.google.com/"));
    preferences.UpdateFromAcceptClientHintsHeader("dpr", kurl, nullptr);
    EXPECT_EQ(use_secure_url,
              preferences.ShouldSend(mojom::WebClientHintsType::kDpr));
  }
}

TEST_F(ClientHintsPreferencesTest, PersistentHints) {
  struct TestCase {
    bool enable_persistent_runtime_feature;
    const char* accept_ch_header_value;
    const char* accept_lifetime_header_value;
    int64_t expect_persist_duration_seconds;
  } test_cases[] = {
      {true, "width, dpr, viewportWidth", "", 0},
      {true, "width, dpr, viewportWidth", "-1000", 0},
      {true, "width, dpr, viewportWidth", "1000s", 0},
      {true, "width, dpr, viewportWidth", "1000.5", 0},
      {false, "width, dpr, viewportWidth", "1000", 0},
      {true, "width, dpr, rtt, downlink, ect", "1000", 1000},
  };

  for (const auto& test : test_cases) {
    WebRuntimeFeatures::EnableClientHintsPersistent(
        test.enable_persistent_runtime_feature);
    WebEnabledClientHints enabled_types;
    TimeDelta persist_duration;

    const KURL kurl(String::FromUTF8("https://www.google.com/"));

    ResourceResponse response(kurl);
    response.SetHTTPHeaderField(HTTPNames::Accept_CH,
                                test.accept_ch_header_value);
    response.SetHTTPHeaderField(HTTPNames::Accept_CH_Lifetime,
                                test.accept_lifetime_header_value);

    ClientHintsPreferences::UpdatePersistentHintsFromHeaders(
        response, nullptr, enabled_types, &persist_duration);
    EXPECT_EQ(test.expect_persist_duration_seconds,
              persist_duration.InSeconds());
    if (test.expect_persist_duration_seconds > 0) {
      EXPECT_FALSE(
          enabled_types.IsEnabled(mojom::WebClientHintsType::kDeviceMemory));
      EXPECT_TRUE(enabled_types.IsEnabled(mojom::WebClientHintsType::kDpr));
      EXPECT_TRUE(
          enabled_types.IsEnabled(mojom::WebClientHintsType::kResourceWidth));
      EXPECT_FALSE(
          enabled_types.IsEnabled(mojom::WebClientHintsType::kViewportWidth));
      EXPECT_TRUE(enabled_types.IsEnabled(mojom::WebClientHintsType::kRtt));
      EXPECT_TRUE(
          enabled_types.IsEnabled(mojom::WebClientHintsType::kDownlink));
      EXPECT_TRUE(enabled_types.IsEnabled(mojom::WebClientHintsType::kEct));
    } else {
      EXPECT_FALSE(
          enabled_types.IsEnabled(mojom::WebClientHintsType::kDeviceMemory));
      EXPECT_FALSE(enabled_types.IsEnabled(mojom::WebClientHintsType::kDpr));
      EXPECT_FALSE(
          enabled_types.IsEnabled(mojom::WebClientHintsType::kResourceWidth));
      EXPECT_FALSE(
          enabled_types.IsEnabled(mojom::WebClientHintsType::kViewportWidth));
      EXPECT_FALSE(enabled_types.IsEnabled(mojom::WebClientHintsType::kRtt));
      EXPECT_FALSE(
          enabled_types.IsEnabled(mojom::WebClientHintsType::kDownlink));
      EXPECT_FALSE(enabled_types.IsEnabled(mojom::WebClientHintsType::kEct));
    }
  }
}

}  // namespace blink
