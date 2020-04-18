// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/warmup_url_fetcher.h"

#include <map>
#include <memory>
#include <vector>

#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/gtest_util.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/platform_thread.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_util.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "net/base/proxy_server.h"
#include "net/http/http_status_code.h"
#include "net/nqe/network_quality_estimator_test_util.h"
#include "net/socket/socket_test_util.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace data_reduction_proxy {

namespace {

class WarmupURLFetcherTest : public WarmupURLFetcher {
 public:
  WarmupURLFetcherTest(const scoped_refptr<net::URLRequestContextGetter>&
                           url_request_context_getter)
      : WarmupURLFetcher(url_request_context_getter,
                         base::BindRepeating(
                             &WarmupURLFetcherTest::HandleWarmupFetcherResponse,
                             base::Unretained(this))) {}

  ~WarmupURLFetcherTest() override {}

  size_t callback_received_count() const { return callback_received_count_; }
  const net::ProxyServer& proxy_server_last() const {
    return proxy_server_last_;
  }
  FetchResult success_response_last() const { return success_response_last_; }

  static void InitExperiment(
      base::test::ScopedFeatureList* scoped_feature_list) {
    std::map<std::string, std::string> params;
    params[params::GetWarmupCallbackParamName()] = "true";
    scoped_feature_list->InitAndEnableFeatureWithParameters(
        features::kDataReductionProxyRobustConnection, params);
  }

  static void InitExperimentWithTimeout(
      base::test::ScopedFeatureList* scoped_feature_list) {
    std::map<std::string, std::string> params;
    params["warmup_fetch_callback_enabled"] = "true";
    params["warmup_url_fetch_min_timeout_seconds"] = "10";
    params["warmup_url_fetch_max_timeout_seconds"] = "60";
    params["warmup_url_fetch_init_http_rtt_multiplier"] = "12";
    scoped_feature_list->InitAndEnableFeatureWithParameters(
        features::kDataReductionProxyRobustConnection, params);
  }

  base::TimeDelta GetFetchWaitTime() const override {
    if (!fetch_wait_time_)
      return WarmupURLFetcher::GetFetchWaitTime();

    return fetch_wait_time_.value();
  }

  void SetFetchWaitTime(base::Optional<base::TimeDelta> fetch_wait_time) {
    fetch_wait_time_ = fetch_wait_time;
  }

  void SetFetchTimeout(base::Optional<base::TimeDelta> fetch_timeout) {
    fetch_timeout_ = fetch_timeout;
  }

  using WarmupURLFetcher::FetchWarmupURL;
  using WarmupURLFetcher::GetWarmupURLWithQueryParam;
  using WarmupURLFetcher::OnFetchTimeout;
  using WarmupURLFetcher::OnURLFetchComplete;

  base::TimeDelta GetFetchTimeout() const override {
    if (!fetch_timeout_)
      return WarmupURLFetcher::GetFetchTimeout();
    return fetch_timeout_.value();
  }

  void VerifyStateCleanedUp() const {
    DCHECK(!fetcher_);
    DCHECK(!fetch_delay_timer_.IsRunning());
    DCHECK(!fetch_timeout_timer_.IsRunning());
    DCHECK(!is_fetch_in_flight_);
  }

  net::URLFetcher* fetcher() const { return fetcher_.get(); }

 private:
  void HandleWarmupFetcherResponse(const net::ProxyServer& proxy_server,
                                   FetchResult success_response) {
    callback_received_count_++;
    proxy_server_last_ = proxy_server;
    success_response_last_ = success_response;
  }

  base::Optional<base::TimeDelta> fetch_wait_time_;
  size_t callback_received_count_ = 0;
  net::ProxyServer proxy_server_last_;
  FetchResult success_response_last_ = FetchResult::kFailed;
  base::Optional<base::TimeDelta> fetch_timeout_;
  DISALLOW_COPY_AND_ASSIGN(WarmupURLFetcherTest);
};

// Test that query param for the warmup URL is randomly set.
TEST(WarmupURLFetcherTest, TestGetWarmupURLWithQueryParam) {
  base::MessageLoopForIO message_loop;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner());
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);

  GURL gurl_original;
  warmup_url_fetcher.GetWarmupURLWithQueryParam(&gurl_original);
  EXPECT_FALSE(gurl_original.query().empty());

  bool query_param_different = false;

  // Generate 5 more GURLs. At least one of them should have a different query
  // param than that of |gurl_original|. Multiple GURLs are generated to
  // probability of test failing due to query params of two GURLs being equal
  // due to chance.
  for (size_t i = 0; i < 5; ++i) {
    GURL gurl;
    warmup_url_fetcher.GetWarmupURLWithQueryParam(&gurl);
    EXPECT_EQ(gurl_original.host(), gurl.host());
    EXPECT_EQ(gurl_original.port(), gurl.port());
    EXPECT_EQ(gurl_original.path(), gurl.path());

    EXPECT_FALSE(gurl.query().empty());

    if (gurl_original.query() != gurl.query())
      query_param_different = true;
  }
  EXPECT_TRUE(query_param_different);
  warmup_url_fetcher.VerifyStateCleanedUp();
}

TEST(WarmupURLFetcherTest, TestSuccessfulFetchWarmupURLNoViaHeader) {
  base::test::ScopedFeatureList scoped_feature_list;
  WarmupURLFetcherTest::InitExperiment(&scoped_feature_list);

  base::HistogramTester histogram_tester;
  base::MessageLoopForIO message_loop;
  const std::string config = "foobarbaz";
  std::vector<std::unique_ptr<net::SocketDataProvider>> socket_data_providers;
  net::MockClientSocketFactory mock_socket_factory;
  net::MockRead success_reads[3];
  success_reads[0] = net::MockRead("HTTP/1.1 200 OK\r\n\r\n");
  success_reads[1] = net::MockRead(net::ASYNC, config.c_str(), config.length());
  success_reads[2] = net::MockRead(net::SYNCHRONOUS, net::OK);

  socket_data_providers.push_back(
      std::make_unique<net::StaticSocketDataProvider>(
          success_reads, base::span<net::MockWrite>()));
  mock_socket_factory.AddSocketDataProvider(socket_data_providers.back().get());

  std::unique_ptr<net::TestURLRequestContext> test_request_context(
      new net::TestURLRequestContext(true));

  test_request_context->set_client_socket_factory(&mock_socket_factory);
  test_request_context->Init();
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner(),
                                           std::move(test_request_context));
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());
  warmup_url_fetcher.FetchWarmupURL(0);
  EXPECT_TRUE(warmup_url_fetcher.IsFetchInFlight());
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());

  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchInitiated", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchSuccessful", 1, 1);
  histogram_tester.ExpectUniqueSample("DataReductionProxy.WarmupURL.NetError",
                                      net::OK, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HttpResponseCode", net::HTTP_OK, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HasViaHeader", 0, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.ProxySchemeUsed",
      util::ConvertNetProxySchemeToProxyScheme(net::ProxyServer::SCHEME_DIRECT),
      1);

  EXPECT_EQ(1u, warmup_url_fetcher.callback_received_count());
  EXPECT_EQ(net::ProxyServer::SCHEME_DIRECT,
            warmup_url_fetcher.proxy_server_last().scheme());
  // success_response_last() should be false since the response does not contain
  // the via header.
  EXPECT_EQ(WarmupURLFetcher::FetchResult::kFailed,
            warmup_url_fetcher.success_response_last());
  warmup_url_fetcher.VerifyStateCleanedUp();
}

TEST(WarmupURLFetcherTest, TestSuccessfulFetchWarmupURLWithViaHeader) {
  base::test::ScopedFeatureList scoped_feature_list;
  WarmupURLFetcherTest::InitExperiment(&scoped_feature_list);

  base::HistogramTester histogram_tester;
  base::MessageLoopForIO message_loop;
  const std::string config = "foobarbaz";
  std::vector<std::unique_ptr<net::SocketDataProvider>> socket_data_providers;
  net::MockClientSocketFactory mock_socket_factory;
  net::MockRead success_reads[3];
  success_reads[0] = net::MockRead(
      "HTTP/1.1 404 NOT FOUND\r\nVia: 1.1 Chrome-Compression-Proxy\r\n\r\n");
  success_reads[1] = net::MockRead(net::ASYNC, config.c_str(), config.length());
  success_reads[2] = net::MockRead(net::SYNCHRONOUS, net::OK);

  socket_data_providers.push_back(
      std::make_unique<net::StaticSocketDataProvider>(
          success_reads, base::span<net::MockWrite>()));
  mock_socket_factory.AddSocketDataProvider(socket_data_providers.back().get());

  std::unique_ptr<net::TestURLRequestContext> test_request_context(
      new net::TestURLRequestContext(true));

  test_request_context->set_client_socket_factory(&mock_socket_factory);
  test_request_context->Init();
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner(),
                                           std::move(test_request_context));
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());
  warmup_url_fetcher.FetchWarmupURL(0);
  EXPECT_TRUE(warmup_url_fetcher.IsFetchInFlight());
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());

  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchInitiated", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchSuccessful", 1, 1);
  histogram_tester.ExpectUniqueSample("DataReductionProxy.WarmupURL.NetError",
                                      net::OK, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HttpResponseCode", net::HTTP_NOT_FOUND, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HasViaHeader", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.ProxySchemeUsed",
      util::ConvertNetProxySchemeToProxyScheme(net::ProxyServer::SCHEME_DIRECT),
      1);

  EXPECT_EQ(1u, warmup_url_fetcher.callback_received_count());
  EXPECT_EQ(net::ProxyServer::SCHEME_DIRECT,
            warmup_url_fetcher.proxy_server_last().scheme());
  // The last response contained the via header.
  EXPECT_EQ(WarmupURLFetcher::FetchResult::kSuccessful,
            warmup_url_fetcher.success_response_last());
  warmup_url_fetcher.VerifyStateCleanedUp();

  // If the fetch times out, it should cause DCHECK to trigger.
  EXPECT_DCHECK_DEATH(warmup_url_fetcher.OnFetchTimeout());
}

TEST(WarmupURLFetcherTest,
     TestSuccessfulFetchWarmupURLWithViaHeaderExperimentNotEnabled) {
  base::HistogramTester histogram_tester;
  base::MessageLoopForIO message_loop;
  const std::string config = "foobarbaz";
  std::vector<std::unique_ptr<net::SocketDataProvider>> socket_data_providers;
  net::MockClientSocketFactory mock_socket_factory;
  net::MockRead success_reads[3];
  success_reads[0] = net::MockRead(
      "HTTP/1.1 204 OK\r\nVia: 1.1 Chrome-Compression-Proxy\r\n\r\n");
  success_reads[1] = net::MockRead(net::ASYNC, config.c_str(), config.length());
  success_reads[2] = net::MockRead(net::SYNCHRONOUS, net::OK);

  socket_data_providers.push_back(
      std::make_unique<net::StaticSocketDataProvider>(
          success_reads, base::span<net::MockWrite>()));
  mock_socket_factory.AddSocketDataProvider(socket_data_providers.back().get());

  std::unique_ptr<net::TestURLRequestContext> test_request_context(
      new net::TestURLRequestContext(true));

  test_request_context->set_client_socket_factory(&mock_socket_factory);
  test_request_context->Init();
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner(),
                                           std::move(test_request_context));
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);
  warmup_url_fetcher.FetchWarmupURL(0);
  base::RunLoop().RunUntilIdle();

  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchInitiated", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchSuccessful", 1, 1);
  histogram_tester.ExpectUniqueSample("DataReductionProxy.WarmupURL.NetError",
                                      net::OK, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HttpResponseCode", net::HTTP_NO_CONTENT, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HasViaHeader", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.ProxySchemeUsed",
      util::ConvertNetProxySchemeToProxyScheme(net::ProxyServer::SCHEME_DIRECT),
      1);

  // The callback should not be run.
  EXPECT_EQ(0u, warmup_url_fetcher.callback_received_count());
  warmup_url_fetcher.VerifyStateCleanedUp();
}

TEST(WarmupURLFetcherTest, TestConnectionResetFetchWarmupURL) {
  base::test::ScopedFeatureList scoped_feature_list;
  WarmupURLFetcherTest::InitExperiment(&scoped_feature_list);

  base::HistogramTester histogram_tester;
  base::MessageLoopForIO message_loop;
  const std::string config = "foobarbaz";
  std::vector<std::unique_ptr<net::SocketDataProvider>> socket_data_providers;
  net::MockClientSocketFactory mock_socket_factory;
  net::MockRead success_reads[1];
  success_reads[0] = net::MockRead(net::SYNCHRONOUS, net::ERR_CONNECTION_RESET);

  socket_data_providers.push_back(
      std::make_unique<net::StaticSocketDataProvider>(
          success_reads, base::span<net::MockWrite>()));
  mock_socket_factory.AddSocketDataProvider(socket_data_providers.back().get());

  std::unique_ptr<net::TestURLRequestContext> test_request_context(
      new net::TestURLRequestContext(true));

  test_request_context->set_client_socket_factory(&mock_socket_factory);
  test_request_context->Init();
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner(),
                                           std::move(test_request_context));
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());
  warmup_url_fetcher.FetchWarmupURL(0);
  EXPECT_TRUE(warmup_url_fetcher.IsFetchInFlight());
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());

  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchInitiated", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchSuccessful", 0, 1);
  histogram_tester.ExpectUniqueSample("DataReductionProxy.WarmupURL.NetError",
                                      std::abs(net::ERR_CONNECTION_RESET), 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HttpResponseCode",
      std::abs(net::URLFetcher::RESPONSE_CODE_INVALID), 1);
  histogram_tester.ExpectTotalCount("DataReductionProxy.WarmupURL.HasViaHeader",
                                    0);
  histogram_tester.ExpectTotalCount(
      "DataReductionProxy.WarmupURL.ProxySchemeUsed", 0);
  EXPECT_EQ(1u, warmup_url_fetcher.callback_received_count());
  EXPECT_EQ(net::ProxyServer::SCHEME_INVALID,
            warmup_url_fetcher.proxy_server_last().scheme());
  EXPECT_EQ(WarmupURLFetcher::FetchResult::kFailed,
            warmup_url_fetcher.success_response_last());
  warmup_url_fetcher.VerifyStateCleanedUp();
}

TEST(WarmupURLFetcherTest, TestFetchTimesout) {
  base::test::ScopedFeatureList scoped_feature_list;
  WarmupURLFetcherTest::InitExperiment(&scoped_feature_list);

  base::HistogramTester histogram_tester;
  base::MessageLoopForIO message_loop;
  const std::string config = "foobarbaz";
  std::vector<std::unique_ptr<net::SocketDataProvider>> socket_data_providers;
  net::MockClientSocketFactory mock_socket_factory;
  net::MockRead success_reads[3];
  success_reads[0] = net::MockRead(
      "HTTP/1.1 204 OK\r\nVia: 1.1 Chrome-Compression-Proxy\r\n\r\n");
  success_reads[1] = net::MockRead(net::ASYNC, config.c_str(), config.length());
  success_reads[2] = net::MockRead(net::SYNCHRONOUS, net::OK);

  socket_data_providers.push_back(
      std::make_unique<net::StaticSocketDataProvider>(
          success_reads, base::span<net::MockWrite>()));
  mock_socket_factory.AddSocketDataProvider(socket_data_providers.back().get());

  std::unique_ptr<net::TestURLRequestContext> test_request_context(
      new net::TestURLRequestContext(true));

  test_request_context->set_client_socket_factory(&mock_socket_factory);
  test_request_context->Init();
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner(),
                                           std::move(test_request_context));
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);
  // Set the timeout to a very low value. This should cause warmup URL fetcher
  // to run the callback with appropriate error code.
  warmup_url_fetcher.SetFetchTimeout(base::TimeDelta::FromSeconds(0));
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());
  warmup_url_fetcher.FetchWarmupURL(0);
  EXPECT_TRUE(warmup_url_fetcher.IsFetchInFlight());
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());

  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchInitiated", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchSuccessful", 0, 1);
  histogram_tester.ExpectUniqueSample("DataReductionProxy.WarmupURL.NetError",
                                      net::ERR_ABORTED, 1);

  EXPECT_EQ(1u, warmup_url_fetcher.callback_received_count());
  // The last response should have timedout.
  EXPECT_EQ(WarmupURLFetcher::FetchResult::kTimedOut,
            warmup_url_fetcher.success_response_last());
  warmup_url_fetcher.VerifyStateCleanedUp();

  // If the URL fetch completes, it should cause DCHECK to trigger.
  EXPECT_DCHECK_DEATH(
      warmup_url_fetcher.OnURLFetchComplete(warmup_url_fetcher.fetcher()));
}

TEST(WarmupURLFetcherTest, TestSuccessfulFetchWarmupURLWithDelay) {
  base::test::ScopedFeatureList scoped_feature_list;
  WarmupURLFetcherTest::InitExperiment(&scoped_feature_list);

  base::HistogramTester histogram_tester;
  base::MessageLoopForIO message_loop;
  const std::string config = "foobarbaz";
  std::vector<std::unique_ptr<net::SocketDataProvider>> socket_data_providers;
  net::MockClientSocketFactory mock_socket_factory;
  net::MockRead success_reads[3];
  success_reads[0] = net::MockRead(
      "HTTP/1.1 404\r\nVia: 1.1 Chrome-Compression-Proxy\r\n\r\n");
  success_reads[1] = net::MockRead(net::ASYNC, config.c_str(), config.length());
  success_reads[2] = net::MockRead(net::SYNCHRONOUS, net::OK);

  socket_data_providers.push_back(
      std::make_unique<net::StaticSocketDataProvider>(
          success_reads, base::span<net::MockWrite>()));
  mock_socket_factory.AddSocketDataProvider(socket_data_providers.back().get());

  std::unique_ptr<net::TestURLRequestContext> test_request_context(
      new net::TestURLRequestContext(true));

  test_request_context->set_client_socket_factory(&mock_socket_factory);
  test_request_context->Init();
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner(),
                                           std::move(test_request_context));
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());
  warmup_url_fetcher.SetFetchWaitTime(base::TimeDelta::FromMilliseconds(1));
  warmup_url_fetcher.FetchWarmupURL(1);
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(2));
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());

  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchInitiated", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.FetchSuccessful", 1, 1);
  histogram_tester.ExpectUniqueSample("DataReductionProxy.WarmupURL.NetError",
                                      net::OK, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HttpResponseCode", net::HTTP_NOT_FOUND, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.HasViaHeader", 1, 1);
  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.WarmupURL.ProxySchemeUsed",
      util::ConvertNetProxySchemeToProxyScheme(net::ProxyServer::SCHEME_DIRECT),
      1);

  EXPECT_EQ(1u, warmup_url_fetcher.callback_received_count());
  EXPECT_EQ(net::ProxyServer::SCHEME_DIRECT,
            warmup_url_fetcher.proxy_server_last().scheme());
  // success_response_last() should be true since the response contains the via
  // header.
  EXPECT_EQ(WarmupURLFetcher::FetchResult::kSuccessful,
            warmup_url_fetcher.success_response_last());
  warmup_url_fetcher.VerifyStateCleanedUp();
}

TEST(WarmupURLFetcherTest, TestFetchTimeoutIncreasing) {
  // Must remain in sync with warmup_url_fetcher.cc.
  constexpr base::TimeDelta kMinTimeout = base::TimeDelta::FromSeconds(8);
  constexpr base::TimeDelta kMaxTimeout = base::TimeDelta::FromSeconds(60);

  base::HistogramTester histogram_tester;
  base::MessageLoopForIO message_loop;

  std::unique_ptr<net::TestURLRequestContext> test_request_context(
      new net::TestURLRequestContext(true));

  test_request_context->Init();
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner(),
                                           std::move(test_request_context));
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());

  EXPECT_EQ(kMinTimeout, warmup_url_fetcher.GetFetchTimeout());

  base::TimeDelta http_rtt = base::TimeDelta::FromSeconds(2);
  estimator.SetStartTimeNullHttpRtt(http_rtt);
  EXPECT_EQ(http_rtt * 5, warmup_url_fetcher.GetFetchTimeout());

  warmup_url_fetcher.FetchWarmupURL(1);
  EXPECT_EQ(http_rtt * 10, warmup_url_fetcher.GetFetchTimeout());

  warmup_url_fetcher.FetchWarmupURL(2);
  EXPECT_EQ(http_rtt * 20, warmup_url_fetcher.GetFetchTimeout());

  http_rtt = base::TimeDelta::FromSeconds(5);
  estimator.SetStartTimeNullHttpRtt(http_rtt);
  EXPECT_EQ(kMaxTimeout, warmup_url_fetcher.GetFetchTimeout());

  warmup_url_fetcher.FetchWarmupURL(0);
  EXPECT_EQ(http_rtt * 5, warmup_url_fetcher.GetFetchTimeout());
}

TEST(WarmupURLFetcherTest, TestFetchTimeoutIncreasingWithFieldTrial) {
  base::test::ScopedFeatureList scoped_feature_list;
  WarmupURLFetcherTest::InitExperimentWithTimeout(&scoped_feature_list);

  // Must remain in sync with InitExperimentWithTimeout().
  constexpr base::TimeDelta kMinTimeout = base::TimeDelta::FromSeconds(10);
  constexpr base::TimeDelta kMaxTimeout = base::TimeDelta::FromSeconds(60);

  base::HistogramTester histogram_tester;
  base::MessageLoopForIO message_loop;

  std::unique_ptr<net::TestURLRequestContext> test_request_context(
      new net::TestURLRequestContext(true));

  test_request_context->Init();
  scoped_refptr<net::URLRequestContextGetter> request_context_getter =
      new net::TestURLRequestContextGetter(message_loop.task_runner(),
                                           std::move(test_request_context));
  net::TestNetworkQualityEstimator estimator;
  request_context_getter->GetURLRequestContext()->set_network_quality_estimator(
      &estimator);

  WarmupURLFetcherTest warmup_url_fetcher(request_context_getter);
  EXPECT_FALSE(warmup_url_fetcher.IsFetchInFlight());

  EXPECT_EQ(kMinTimeout, warmup_url_fetcher.GetFetchTimeout());

  base::TimeDelta http_rtt = base::TimeDelta::FromSeconds(1);
  estimator.SetStartTimeNullHttpRtt(http_rtt);
  EXPECT_EQ(http_rtt * 12, warmup_url_fetcher.GetFetchTimeout());

  warmup_url_fetcher.FetchWarmupURL(1);
  EXPECT_EQ(http_rtt * 24, warmup_url_fetcher.GetFetchTimeout());

  warmup_url_fetcher.FetchWarmupURL(2);
  EXPECT_EQ(http_rtt * 48, warmup_url_fetcher.GetFetchTimeout());

  http_rtt = base::TimeDelta::FromSeconds(5);
  estimator.SetStartTimeNullHttpRtt(http_rtt);
  EXPECT_EQ(kMaxTimeout, warmup_url_fetcher.GetFetchTimeout());

  warmup_url_fetcher.FetchWarmupURL(0);
  EXPECT_EQ(http_rtt * 12, warmup_url_fetcher.GetFetchTimeout());
}

}  // namespace

}  // namespace data_reduction_proxy
