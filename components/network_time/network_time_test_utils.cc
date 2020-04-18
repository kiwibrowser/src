// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/network_time/network_time_test_utils.h"

#include <memory>

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/mock_entropy_provider.h"
#include "base/test/scoped_feature_list.h"
#include "components/variations/variations_associated_data.h"
#include "net/http/http_response_headers.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network_time {

// Update as follows:
//
// curl -i http://clients2.google.com/time/1/current?cup2key=2:123123123
//
// where 2 is the key version and 123123123 is the nonce.  Copy the
// response and the x-cup-server-proof header into
// |kGoodTimeResponseBody| and |kGoodTimeResponseServerProofHeader|
// respectively, and the 'current_time_millis' value of the response
// into |kGoodTimeResponseHandlerJsTime|.
const char* kGoodTimeResponseBody[] = {
    ")]}'\n{\"current_time_millis\":1522081016324,"
    "\"server_nonce\":-1.475187036492045E154}",
    ")]}'\n{\"current_time_millis\":1522096305984,"
    "\"server_nonce\":-1.1926302260014708E-276}"};
const char* kGoodTimeResponseServerProofHeader[] = {
    "3046022100c0351a20558bac037253f3969547f82805b340f51de06461e83f33b41f8e85d3"
    "022100d04162c448438e5462df4bf6171ef26c53ec7d3a0cb915409e8bec6c99c69c67:"
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    "304402201758cc66f7be58692362dad351ee71ecce78bd8491c8bfe903da39ea048ff67d02"
    "203aa51acfac9462b19ef3e6d6c885a60cb0858a274ae97506934737d8e66bc081:"
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};
const double kGoodTimeResponseHandlerJsTime[] = {1522081016324, 1522096305984};

std::unique_ptr<net::test_server::HttpResponse> GoodTimeResponseHandler(
    const net::test_server::HttpRequest& request) {
  net::test_server::BasicHttpResponse* response =
      new net::test_server::BasicHttpResponse();
  response->set_code(net::HTTP_OK);
  response->set_content(kGoodTimeResponseBody[0]);
  response->AddCustomHeader("x-cup-server-proof",
                            kGoodTimeResponseServerProofHeader[0]);
  return std::unique_ptr<net::test_server::HttpResponse>(response);
}

FieldTrialTest::FieldTrialTest() {}

FieldTrialTest::~FieldTrialTest() {}

void FieldTrialTest::SetNetworkQueriesWithVariationsService(
    bool enable,
    float query_probability,
    NetworkTimeTracker::FetchBehavior fetch_behavior) {
  const std::string kTrialName = "Trial";
  const std::string kGroupName = "group";
  const base::Feature kFeature{"NetworkTimeServiceQuerying",
                               base::FEATURE_DISABLED_BY_DEFAULT};

  // Clear all the things.
  variations::testing::ClearAllVariationParams();

  std::map<std::string, std::string> params;
  params["RandomQueryProbability"] = base::NumberToString(query_probability);
  params["CheckTimeIntervalSeconds"] = base::Int64ToString(360);
  std::string fetch_behavior_param;
  switch (fetch_behavior) {
    case NetworkTimeTracker::FETCH_BEHAVIOR_UNKNOWN:
      NOTREACHED();
      fetch_behavior_param = "unknown";
      break;
    case NetworkTimeTracker::FETCHES_IN_BACKGROUND_ONLY:
      fetch_behavior_param = "background-only";
      break;
    case NetworkTimeTracker::FETCHES_ON_DEMAND_ONLY:
      fetch_behavior_param = "on-demand-only";
      break;
    case NetworkTimeTracker::FETCHES_IN_BACKGROUND_AND_ON_DEMAND:
      fetch_behavior_param = "background-and-on-demand";
      break;
  }
  params["FetchBehavior"] = fetch_behavior_param;

  // There are 3 things here: a FieldTrial, a FieldTrialList, and a
  // FeatureList.  Don't get confused!  The FieldTrial is reference-counted,
  // and a reference is held by the FieldTrialList.  The FieldTrialList and
  // FeatureList are both singletons.  The authorized way to reset the former
  // for testing is to destruct it (above).  The latter, by contrast, should
  // should already start in a clean state and can be manipulated via the
  // ScopedFeatureList helper class. If this comment was useful to you
  // please send me a postcard.

  // SetNetworkQueriesWithVariationsService() is usually called during test
  // fixture setup (to establish a default state) and then again in certain
  // tests that want to set special params. FieldTrialList is meant to be a
  // singleton with only one instance existing at once, and the constructor
  // fails a CHECK if this is violated. To allow these duplicate calls to this
  // method, any existing FieldTrialList must be destroyed before creating a new
  // one. (See https://crbug.com/684216#c5 for more discussion.)
  field_trial_list_.reset();
  field_trial_list_.reset(
      new base::FieldTrialList(std::make_unique<base::MockEntropyProvider>()));

  // refcounted, and reference held by the singleton FieldTrialList.
  base::FieldTrial* trial = base::FieldTrialList::FactoryGetFieldTrial(
      kTrialName, 100, kGroupName, 1971, 1, 1,
      base::FieldTrial::SESSION_RANDOMIZED, nullptr /* default_group_number */);
  ASSERT_TRUE(
      variations::AssociateVariationParams(kTrialName, kGroupName, params));

  std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
  feature_list->RegisterFieldTrialOverride(
      kFeature.name, enable ? base::FeatureList::OVERRIDE_ENABLE_FEATURE
                            : base::FeatureList::OVERRIDE_DISABLE_FEATURE,
      trial);
  scoped_feature_list_.reset(new base::test::ScopedFeatureList);
  scoped_feature_list_->InitWithFeatureList(std::move(feature_list));
}

}  // namespace network_time
