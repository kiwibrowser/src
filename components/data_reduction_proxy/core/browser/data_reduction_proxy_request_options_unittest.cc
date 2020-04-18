// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_request_options.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/md5.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config_test_utils.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_test_utils.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params_test_utils.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_switches.h"
#include "components/variations/variations_associated_data.h"
#include "net/base/auth.h"
#include "net/base/host_port_pair.h"
#include "net/base/load_flags.h"
#include "net/base/proxy_server.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {
const char kChromeProxyHeader[] = "chrome-proxy";

const char kVersion[] = "0.1.2.3";
const char kExpectedBuild[] = "2";
const char kExpectedPatch[] = "3";
const char kExpectedCredentials[] = "96bd72ec4a050ba60981743d41787768";
const char kExpectedSession[] = "0-1633771873-1633771873-1633771873";
const char kPageId[] = "1";
const uint64_t kPageIdValue = 1;

const char kTestKey2[] = "test-key2";
const char kExpectedCredentials2[] = "c911fdb402f578787562cf7f00eda972";
const char kExpectedSession2[] = "0-1633771873-1633771873-1633771873";
const char kDataReductionProxyKey[] = "12345";
const char kPageId2[] = "f";
const uint64_t kPageIdValue2 = 15;

const char kSecureSession[] = "TestSecureSessionKey";
}  // namespace


namespace data_reduction_proxy {
namespace {

#if defined(OS_ANDROID)
const Client kClient = Client::CHROME_ANDROID;
const char kClientStr[] = "android";
#elif defined(OS_IOS)
const Client kClient = Client::CHROME_IOS;
const char kClientStr[] = "ios";
#elif defined(OS_MACOSX)
const Client kClient = Client::CHROME_MAC;
const char kClientStr[] = "mac";
#elif defined(OS_CHROMEOS)
const Client kClient = Client::CHROME_CHROMEOS;
const char kClientStr[] = "chromeos";
#elif defined(OS_LINUX)
const Client kClient = Client::CHROME_LINUX;
const char kClientStr[] = "linux";
#elif defined(OS_WIN)
const Client kClient = Client::CHROME_WINDOWS;
const char kClientStr[] = "win";
#elif defined(OS_FREEBSD)
const Client kClient = Client::CHROME_FREEBSD;
const char kClientStr[] = "freebsd";
#elif defined(OS_OPENBSD)
const Client kClient = Client::CHROME_OPENBSD;
const char kClientStr[] = "openbsd";
#elif defined(OS_SOLARIS)
const Client kClient = Client::CHROME_SOLARIS;
const char kClientStr[] = "solaris";
#elif defined(OS_QNX)
const Client kClient = Client::CHROME_QNX;
const char kClientStr[] = "qnx";
#else
const Client kClient = Client::UNKNOWN;
const char kClientStr[] = "";
#endif

void SetHeaderExpectations(const std::string& session,
                           const std::string& credentials,
                           const std::string& secure_session,
                           const std::string& client,
                           const std::string& build,
                           const std::string& patch,
                           const std::string& page_id,
                           const std::vector<std::string> experiments,
                           std::string* expected_header) {
  std::vector<std::string> expected_options;
  if (!session.empty()) {
    expected_options.push_back(
        std::string(kSessionHeaderOption) + "=" + session);
  }
  if (!credentials.empty()) {
    expected_options.push_back(
        std::string(kCredentialsHeaderOption) + "=" + credentials);
  }
  if (!secure_session.empty()) {
    expected_options.push_back(std::string(kSecureSessionHeaderOption) + "=" +
                               secure_session);
  }
  if (!client.empty()) {
    expected_options.push_back(
        std::string(kClientHeaderOption) + "=" + client);
  }
  EXPECT_FALSE(build.empty());
  expected_options.push_back(std::string(kBuildNumberHeaderOption) + "=" +
                             build);
  EXPECT_FALSE(patch.empty());
  expected_options.push_back(std::string(kPatchNumberHeaderOption) + "=" +
                             patch);

  for (const auto& experiment : experiments) {
    expected_options.push_back(
        std::string(kExperimentsOption) + "=" + experiment);
  }

  EXPECT_FALSE(page_id.empty());
  expected_options.push_back("pid=" + page_id);

  if (!expected_options.empty())
    *expected_header = base::JoinString(expected_options, ", ");
}

}  // namespace

class DataReductionProxyRequestOptionsTest : public testing::Test {
 public:
  DataReductionProxyRequestOptionsTest() {
    test_context_ =
        DataReductionProxyTestContext::Builder()
            .Build();
  }

  void CreateRequestOptions(const std::string& version) {
    request_options_.reset(new TestDataReductionProxyRequestOptions(
        kClient, version, test_context_->config()));
    request_options_->Init();
  }

  TestDataReductionProxyParams* params() {
    return test_context_->config()->test_params();
  }

  TestDataReductionProxyRequestOptions* request_options() {
    return request_options_.get();
  }

  void VerifyExpectedHeader(const std::string& expected_header,
                            uint64_t page_id) {
    test_context_->RunUntilIdle();
    net::HttpRequestHeaders headers;
    request_options_->AddRequestHeader(&headers, page_id);
    if (expected_header.empty()) {
      EXPECT_FALSE(headers.HasHeader(kChromeProxyHeader));
      return;
    }
    EXPECT_TRUE(headers.HasHeader(kChromeProxyHeader));
    std::string header_value;
    headers.GetHeader(kChromeProxyHeader, &header_value);
    EXPECT_EQ(expected_header, header_value);
  }

  base::MessageLoopForIO message_loop_;
  std::unique_ptr<TestDataReductionProxyRequestOptions> request_options_;
  std::unique_ptr<DataReductionProxyTestContext> test_context_;
};

TEST_F(DataReductionProxyRequestOptionsTest, AuthHashForSalt) {
  std::string salt = "8675309"; // Jenny's number to test the hash generator.
  std::string salted_key = salt + kDataReductionProxyKey + salt;
  base::string16 expected_hash = base::UTF8ToUTF16(base::MD5String(salted_key));
  EXPECT_EQ(expected_hash,
            DataReductionProxyRequestOptions::AuthHashForSalt(
                8675309, kDataReductionProxyKey));
}

TEST_F(DataReductionProxyRequestOptionsTest, AuthorizationOnIOThread) {
  std::string expected_header;
  SetHeaderExpectations(kExpectedSession2, kExpectedCredentials2, std::string(),
                        kClientStr, kExpectedBuild, kExpectedPatch, kPageId,
                        std::vector<std::string>(), &expected_header);

  std::string expected_header2;
  SetHeaderExpectations("86401-1633771873-1633771873-1633771873",
                        "d7c1c34ef6b90303b01c48a6c1db6419", std::string(),
                        kClientStr, kExpectedBuild, kExpectedPatch, kPageId2,
                        std::vector<std::string>(), &expected_header2);

  CreateRequestOptions(kVersion);
  test_context_->RunUntilIdle();

  // Now set a key.
  request_options()->SetKeyOnIO(kTestKey2);

  // Write headers.
  VerifyExpectedHeader(expected_header, kPageIdValue);

  // Fast forward 24 hours. The header should be the same.
  request_options()->set_offset(base::TimeDelta::FromSeconds(24 * 60 * 60));
  VerifyExpectedHeader(expected_header, kPageIdValue);

  // Fast forward one more second. The header should be new.
  request_options()->set_offset(base::TimeDelta::FromSeconds(24 * 60 * 60 + 1));
  VerifyExpectedHeader(expected_header2, kPageIdValue2);
}

TEST_F(DataReductionProxyRequestOptionsTest, AuthorizationIgnoresEmptyKey) {
  std::string expected_header;
  SetHeaderExpectations(kExpectedSession, kExpectedCredentials, std::string(),
                        kClientStr, kExpectedBuild, kExpectedPatch, kPageId,
                        std::vector<std::string>(), &expected_header);
  CreateRequestOptions(kVersion);
  VerifyExpectedHeader(expected_header, kPageIdValue);

  // Now set an empty key. The auth handler should ignore that, and the key
  // remains |kTestKey|.
  request_options()->SetKeyOnIO(std::string());
  VerifyExpectedHeader(expected_header, kPageIdValue);
}

TEST_F(DataReductionProxyRequestOptionsTest, SecureSession) {
  std::string expected_header;
  SetHeaderExpectations(std::string(), std::string(), kSecureSession,
                        kClientStr, kExpectedBuild, kExpectedPatch, kPageId,
                        std::vector<std::string>(), &expected_header);

  CreateRequestOptions(kVersion);
  request_options()->SetSecureSession(kSecureSession);
  VerifyExpectedHeader(expected_header, kPageIdValue);
}

TEST_F(DataReductionProxyRequestOptionsTest, ParseExperiments) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      data_reduction_proxy::switches::kDataReductionProxyExperiment,
      "staging,\"foo,bar\"");
  std::vector<std::string> expected_experiments;
  expected_experiments.push_back("staging");
  expected_experiments.push_back("\"foo,bar\"");
  std::string expected_header;
  SetHeaderExpectations(kExpectedSession, kExpectedCredentials, std::string(),
                        kClientStr, kExpectedBuild, kExpectedPatch, kPageId,
                        expected_experiments, &expected_header);

  CreateRequestOptions(kVersion);
  VerifyExpectedHeader(expected_header, kPageIdValue);
}

TEST_F(DataReductionProxyRequestOptionsTest, ParseExperimentsFromFieldTrial) {
  const char kFieldTrialGroupFoo[] = "enabled_foo";
  const char kFieldTrialGroupBar[] = "enabled_bar";
  const char kExperimentFoo[] = "foo";
  const char kExperimentBar[] = "bar";
  const struct {
    std::string field_trial_group;
    std::string command_line_experiment;
    bool disable_server_experiments_via_flag;
    std::string expected_experiment;
  } tests[] = {
      // Disabled field trial groups.
      {"disabled_group", std::string(), false, std::string()},
      {"disabled_group", kExperimentFoo, false, kExperimentFoo},
      // Valid field trial groups should pick from field trial.
      {kFieldTrialGroupFoo, std::string(), false, kExperimentFoo},
      {kFieldTrialGroupBar, std::string(), false, kExperimentBar},
      {kFieldTrialGroupFoo, std::string(), true, std::string()},
      {kFieldTrialGroupBar, std::string(), true, std::string()},
      // Experiments from command line switch should override.
      {kFieldTrialGroupFoo, kExperimentBar, false, kExperimentBar},
      {kFieldTrialGroupBar, kExperimentFoo, false, kExperimentFoo},
      {kFieldTrialGroupFoo, kExperimentBar, true, kExperimentBar},
      {kFieldTrialGroupBar, kExperimentFoo, true, kExperimentFoo},
  };

  std::map<std::string, std::string> server_experiment_foo,
      server_experiment_bar;

  server_experiment_foo["exp"] = kExperimentFoo;
  server_experiment_bar["exp"] = kExperimentBar;
  ASSERT_TRUE(variations::AssociateVariationParams(
      params::GetServerExperimentsFieldTrialName(), kFieldTrialGroupFoo,
      server_experiment_foo));
  ASSERT_TRUE(variations::AssociateVariationParams(
      params::GetServerExperimentsFieldTrialName(), kFieldTrialGroupBar,
      server_experiment_bar));

  for (const auto& test : tests) {
    std::vector<std::string> expected_experiments;

    base::CommandLine::ForCurrentProcess()->InitFromArgv(0, nullptr);

    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        data_reduction_proxy::switches::kDataReductionProxyExperiment,
        test.command_line_experiment);
    if (test.disable_server_experiments_via_flag) {
      base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
          switches::kDataReductionProxyServerExperimentsDisabled, "");
    }

    std::string expected_header;
    base::FieldTrialList field_trial_list(nullptr);
    base::FieldTrialList::CreateFieldTrial(
        params::GetServerExperimentsFieldTrialName(), test.field_trial_group);

    if (!test.expected_experiment.empty())
      expected_experiments.push_back(test.expected_experiment);

    SetHeaderExpectations(kExpectedSession, kExpectedCredentials, std::string(),
                          kClientStr, kExpectedBuild, kExpectedPatch, kPageId,
                          expected_experiments, &expected_header);

    CreateRequestOptions(kVersion);
    VerifyExpectedHeader(expected_header, kPageIdValue);
  }
}

TEST_F(DataReductionProxyRequestOptionsTest, TestExperimentPrecedence) {
  // Tests that combinations of configurations that trigger "exp=" directive in
  // the Chrome-Proxy header have the right precendence, and only append a value
  // for the highest priority value.

  // Field trial has the lowest priority.
  std::map<std::string, std::string> server_experiment;
  server_experiment["exp"] = "foo";
  ASSERT_TRUE(variations::AssociateVariationParams(
      params::GetServerExperimentsFieldTrialName(), "enabled",
      server_experiment));

  base::FieldTrialList field_trial_list(nullptr);
  base::FieldTrialList::CreateFieldTrial(
      params::GetServerExperimentsFieldTrialName(), "enabled");
  std::vector<std::string> expected_experiments;
  expected_experiments.push_back("foo");
  std::string expected_header;
  SetHeaderExpectations(kExpectedSession, kExpectedCredentials, std::string(),
                        kClientStr, kExpectedBuild, kExpectedPatch, kPageId,
                        expected_experiments, &expected_header);
  CreateRequestOptions(kVersion);
  VerifyExpectedHeader(expected_header, kPageIdValue);

  // Setting the experiment explicitly has the highest priority.
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      data_reduction_proxy::switches::kDataReductionProxyExperiment, "bar");
  expected_experiments.clear();
  expected_experiments.push_back("bar");
  SetHeaderExpectations(kExpectedSession, kExpectedCredentials, std::string(),
                        kClientStr, kExpectedBuild, kExpectedPatch, kPageId,
                        expected_experiments, &expected_header);
  CreateRequestOptions(kVersion);
  VerifyExpectedHeader(expected_header, kPageIdValue);
}

TEST_F(DataReductionProxyRequestOptionsTest, GetSessionKeyFromRequestHeaders) {
  const struct {
    std::string chrome_proxy_header_key;
    std::string chrome_proxy_header_value;
    std::string expected_session_key;
  } tests[] = {
      {"chrome-proxy", "something=something_else, s=123, key=value", "123"},
      {"chrome-proxy", "something=something_else, s= 123  456 , key=value",
       "123  456"},
      {"chrome-proxy", "something=something_else, s=123456,    key=value",
       "123456"},
      {"chrome-proxy", "something=something else, s=123456,    key=value",
       "123456"},
      {"chrome-proxy", "something=something else, s=123456  ", "123456"},
      {"chrome-proxy", "something=something_else, s=, key=value", ""},
      {"chrome-proxy", "something=something_else, key=value", ""},
      {"chrome-proxy", "s=123", "123"},
      {"chrome-proxy", " s = 123 ", "123"},
      {"some_other_header", "s=123", ""},
  };

  for (const auto& test : tests) {
    net::HttpRequestHeaders request_headers;
    request_headers.SetHeader("some_random_header_before", "some_random_key");
    request_headers.SetHeader(test.chrome_proxy_header_key,
                              test.chrome_proxy_header_value);
    request_headers.SetHeader("some_random_header_after", "some_random_key");

    std::string session_key =
        request_options()->GetSessionKeyFromRequestHeaders(request_headers);
    EXPECT_EQ(test.expected_session_key, session_key)
        << test.chrome_proxy_header_key << ":"
        << test.chrome_proxy_header_value;
  }
}

TEST_F(DataReductionProxyRequestOptionsTest, PageIdIncrementing) {
  CreateRequestOptions(kVersion);
  uint64_t page_id = request_options()->GeneratePageId();
  DCHECK_EQ(++page_id, request_options()->GeneratePageId());
  DCHECK_EQ(++page_id, request_options()->GeneratePageId());
  DCHECK_EQ(++page_id, request_options()->GeneratePageId());

  request_options()->SetSecureSession("blah");

  page_id = request_options()->GeneratePageId();
  DCHECK_EQ(++page_id, request_options()->GeneratePageId());
  DCHECK_EQ(++page_id, request_options()->GeneratePageId());
  DCHECK_EQ(++page_id, request_options()->GeneratePageId());
}

}  // namespace data_reduction_proxy
