// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/policy_header_service.h"

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/message_loop/message_loop.h"
#include "base/test/test_simple_task_runner.h"
#include "base/values.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/core/common/cloud/mock_cloud_policy_store.h"
#include "components/policy/core/common/cloud/policy_header_io_helper.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {
using enterprise_management::PolicyData;

namespace {
const char kDMServerURL[] = "http://server_url";
const char kPolicyHeaderName[] = "Chrome-Policy-Posture";

class TestCloudPolicyStore : public MockCloudPolicyStore {
 public:
  void SetPolicy(std::unique_ptr<PolicyData> policy) {
    policy_ = std::move(policy);
    // Notify observers.
    NotifyStoreLoaded();
  }
};

class PolicyHeaderServiceTest : public testing::Test {
 public:
  PolicyHeaderServiceTest() {
    task_runner_ = base::MakeRefCounted<base::TestSimpleTaskRunner>();
  }
  ~PolicyHeaderServiceTest() override {}

  void SetUp() override {
    service_.reset(new PolicyHeaderService(kDMServerURL,
                                           kPolicyVerificationKeyHash,
                                           &user_store_));
    helper_ = service_->CreatePolicyHeaderIOHelper(task_runner_);
  }

  void TearDown() override {
    task_runner_->RunUntilIdle();
    // Helper should outlive the service.
    service_.reset();
    helper_.reset();
  }

  void ValidateHeader(const net::HttpRequestHeaders& headers,
                      const std::string& expected_dmtoken,
                      const std::string& expected_policy_token) {
    if (expected_dmtoken.empty()) {
      EXPECT_TRUE(headers.IsEmpty());
    } else {
      // Read header.
      std::string header;
      EXPECT_TRUE(headers.GetHeader(kPolicyHeaderName, &header));
      // Decode the base64 value into JSON.
      std::string decoded;
      base::Base64Decode(header, &decoded);
      // Parse the JSON.
      std::unique_ptr<base::Value> value = base::JSONReader::Read(decoded);
      ASSERT_TRUE(value);
      base::DictionaryValue* dict;
      EXPECT_TRUE(value->GetAsDictionary(&dict));
      // Read the values and verify them vs the expected values.
      std::string dm_token;
      dict->GetString("user_dmtoken", &dm_token);
      EXPECT_EQ(dm_token, expected_dmtoken);
      std::string policy_token;
      dict->GetString("user_policy_token", &policy_token);
      EXPECT_EQ(policy_token, expected_policy_token);
    }
  }

  base::MessageLoop loop_;
  std::unique_ptr<PolicyHeaderService> service_;
  TestCloudPolicyStore user_store_;
  std::unique_ptr<PolicyHeaderIOHelper> helper_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
};

}  // namespace

TEST_F(PolicyHeaderServiceTest, TestCreationAndShutdown) {
  // Just tests that the objects can be created and shutdown properly.
  EXPECT_TRUE(service_);
  EXPECT_TRUE(helper_);
}

TEST_F(PolicyHeaderServiceTest, TestWithAndWithoutPolicyHeader) {
  // Set policy - this should push a header to the PolicyHeaderIOHelper.
  std::unique_ptr<PolicyData> policy(new PolicyData());
  std::string expected_dmtoken = "expected_dmtoken";
  std::string expected_policy_token = "expected_dmtoken";
  policy->set_request_token(expected_dmtoken);
  policy->set_policy_token(expected_policy_token);
  user_store_.SetPolicy(std::move(policy));
  task_runner_->RunUntilIdle();

  net::TestURLRequestContext context;
  std::unique_ptr<net::URLRequest> request(
      context.CreateRequest(GURL(kDMServerURL), net::DEFAULT_PRIORITY, nullptr,
                            TRAFFIC_ANNOTATION_FOR_TESTS));
  helper_->AddPolicyHeaders(request->url(), request.get());
  ValidateHeader(request->extra_request_headers(), expected_dmtoken,
                 expected_policy_token);

  // Now blow away the policy data.
  user_store_.SetPolicy(std::unique_ptr<PolicyData>());
  task_runner_->RunUntilIdle();

  std::unique_ptr<net::URLRequest> request2(
      context.CreateRequest(GURL(kDMServerURL), net::DEFAULT_PRIORITY, nullptr,
                            TRAFFIC_ANNOTATION_FOR_TESTS));
  helper_->AddPolicyHeaders(request2->url(), request2.get());
  ValidateHeader(request2->extra_request_headers(), "", "");
}

}  // namespace policy
