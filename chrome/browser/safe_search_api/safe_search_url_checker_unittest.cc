// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_search_api/safe_search_url_checker.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using Classification = SafeSearchURLChecker::Classification;
using testing::_;

namespace {

const size_t kCacheSize = 2;

const char* kURLs[] = {
    "http://www.randomsite1.com",
    "http://www.randomsite2.com",
    "http://www.randomsite3.com",
    "http://www.randomsite4.com",
    "http://www.randomsite5.com",
    "http://www.randomsite6.com",
    "http://www.randomsite7.com",
    "http://www.randomsite8.com",
    "http://www.randomsite9.com",
};

const char kSafeSearchApiUrl[] =
    "https://safesearch.googleapis.com/v1:classify";

std::string BuildResponse(bool is_porn) {
  base::DictionaryValue dict;
  std::unique_ptr<base::DictionaryValue> classification_dict(
      new base::DictionaryValue);
  if (is_porn)
    classification_dict->SetBoolean("pornography", is_porn);
  auto classifications_list = std::make_unique<base::ListValue>();
  classifications_list->Append(std::move(classification_dict));
  dict.SetWithoutPathExpansion("classifications",
                               std::move(classifications_list));
  std::string result;
  base::JSONWriter::Write(dict, &result);
  return result;
}

}  // namespace

class SafeSearchURLCheckerTest : public testing::Test {
 public:
  SafeSearchURLCheckerTest()
      : next_url_(0),
        test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)),
        checker_(test_shared_loader_factory_,
                 TRAFFIC_ANNOTATION_FOR_TESTS,
                 kCacheSize) {}

  MOCK_METHOD3(OnCheckDone,
               void(const GURL& url,
                    Classification classification,
                    bool uncertain));

 protected:
  GURL GetNewURL() {
    CHECK(next_url_ < arraysize(kURLs));
    return GURL(kURLs[next_url_++]);
  }

  void SetupResponse(const GURL& url,
                     net::Error error,
                     const std::string& response) {
    network::URLLoaderCompletionStatus status(error);
    status.decoded_body_length = response.size();
    test_url_loader_factory_.AddResponse(GURL(kSafeSearchApiUrl),
                                         network::ResourceResponseHead(),
                                         response, status);
  }

  // Returns true if the result was returned synchronously (cache hit).
  bool CheckURL(const GURL& url) {
    bool cached = checker_.CheckURL(
        url, base::BindOnce(&SafeSearchURLCheckerTest::OnCheckDone,
                            base::Unretained(this)));
    return cached;
  }

  void WaitForResponse() { base::RunLoop().RunUntilIdle(); }

  bool SendValidResponse(const GURL& url, bool is_porn) {
    SetupResponse(url, net::OK, BuildResponse(is_porn));
    bool result = CheckURL(url);
    WaitForResponse();
    return result;
  }

  bool SendFailedResponse(const GURL& url) {
    SetupResponse(url, net::ERR_ABORTED, std::string());
    bool result = CheckURL(url);
    WaitForResponse();
    return result;
  }

  size_t next_url_;
  base::MessageLoop message_loop_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
  SafeSearchURLChecker checker_;
};

TEST_F(SafeSearchURLCheckerTest, Simple) {
  {
    GURL url(GetNewURL());
    EXPECT_CALL(*this, OnCheckDone(url, Classification::SAFE, false));
    ASSERT_FALSE(SendValidResponse(url, false));
  }
  {
    GURL url(GetNewURL());
    EXPECT_CALL(*this, OnCheckDone(url, Classification::UNSAFE, false));
    ASSERT_FALSE(SendValidResponse(url, true));
  }
  {
    GURL url(GetNewURL());
    EXPECT_CALL(*this, OnCheckDone(url, Classification::SAFE, true));
    ASSERT_FALSE(SendFailedResponse(url));
  }
}

TEST_F(SafeSearchURLCheckerTest, Cache) {
  // One more URL than fit in the cache.
  ASSERT_EQ(2u, kCacheSize);
  GURL url1(GetNewURL());
  GURL url2(GetNewURL());
  GURL url3(GetNewURL());

  // Populate the cache.
  EXPECT_CALL(*this, OnCheckDone(url1, Classification::SAFE, false));
  ASSERT_FALSE(SendValidResponse(url1, false));
  EXPECT_CALL(*this, OnCheckDone(url2, Classification::SAFE, false));
  ASSERT_FALSE(SendValidResponse(url2, false));

  // Now we should get results synchronously, without a network request.
  test_url_loader_factory_.ClearResponses();
  EXPECT_CALL(*this, OnCheckDone(url2, Classification::SAFE, false));
  ASSERT_TRUE(CheckURL(url2));
  EXPECT_CALL(*this, OnCheckDone(url1, Classification::SAFE, false));
  ASSERT_TRUE(CheckURL(url1));

  // Now |url2| is the LRU and should be evicted on the next check.
  EXPECT_CALL(*this, OnCheckDone(url3, Classification::SAFE, false));
  ASSERT_FALSE(SendValidResponse(url3, false));

  EXPECT_CALL(*this, OnCheckDone(url2, Classification::SAFE, false));
  ASSERT_FALSE(SendValidResponse(url2, false));
}

TEST_F(SafeSearchURLCheckerTest, CoalesceRequestsToSameURL) {
  GURL url(GetNewURL());
  // Start two checks for the same URL.
  SetupResponse(url, net::OK, BuildResponse(false));
  ASSERT_FALSE(CheckURL(url));
  ASSERT_FALSE(CheckURL(url));
  // A single response should answer both of those checks
  EXPECT_CALL(*this, OnCheckDone(url, Classification::SAFE, false)).Times(2);
  WaitForResponse();
}

TEST_F(SafeSearchURLCheckerTest, CacheTimeout) {
  GURL url(GetNewURL());

  checker_.SetCacheTimeoutForTesting(base::TimeDelta::FromSeconds(0));

  EXPECT_CALL(*this, OnCheckDone(url, Classification::SAFE, false));
  ASSERT_FALSE(SendValidResponse(url, false));

  // Since the cache timeout is zero, the cache entry should be invalidated
  // immediately.
  EXPECT_CALL(*this, OnCheckDone(url, Classification::UNSAFE, false));
  ASSERT_FALSE(SendValidResponse(url, true));
}
