// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/preconnect_manager.h"

#include <utility>

#include "base/format_macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/predictors/resource_prefetch_predictor.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/proxy_resolution/mock_proxy_resolver.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::DoAll;
using testing::Mock;
using testing::Return;
using testing::SaveArg;
using testing::StrictMock;

namespace predictors {

class MockPreconnectManagerDelegate
    : public PreconnectManager::Delegate,
      public base::SupportsWeakPtr<MockPreconnectManagerDelegate> {
 public:
  // Gmock doesn't support mocking methods with move-only argument types.
  void PreconnectFinished(std::unique_ptr<PreconnectStats> stats) override {
    PreconnectFinishedProxy(stats->url);
  }

  MOCK_METHOD1(PreconnectFinishedProxy, void(const GURL& url));
};

class MockPreconnectManager : public PreconnectManager {
 public:
  MockPreconnectManager(
      base::WeakPtr<Delegate> delegate,
      scoped_refptr<net::URLRequestContextGetter> context_getter);

  MOCK_CONST_METHOD4(PreconnectUrl,
                     void(const GURL& url,
                          const GURL& site_for_cookies,
                          int num_sockets,
                          bool allow_credentials));
  MOCK_CONST_METHOD2(PreresolveUrl,
                     int(const GURL& url,
                         const net::CompletionCallback& callback));
};

MockPreconnectManager::MockPreconnectManager(
    base::WeakPtr<Delegate> delegate,
    scoped_refptr<net::URLRequestContextGetter> context_getter)
    : PreconnectManager(delegate, context_getter) {}

class PreconnectManagerTest : public testing::Test {
 public:
  PreconnectManagerTest();
  ~PreconnectManagerTest() override;

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<StrictMock<MockPreconnectManagerDelegate>> mock_delegate_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;
  std::unique_ptr<StrictMock<MockPreconnectManager>> preconnect_manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PreconnectManagerTest);
};

PreconnectManagerTest::PreconnectManagerTest()
    : mock_delegate_(
          std::make_unique<StrictMock<MockPreconnectManagerDelegate>>()),
      context_getter_(base::MakeRefCounted<net::TestURLRequestContextGetter>(
          base::ThreadTaskRunnerHandle::Get())),
      preconnect_manager_(std::make_unique<StrictMock<MockPreconnectManager>>(
          mock_delegate_->AsWeakPtr(),
          context_getter_)) {}

PreconnectManagerTest::~PreconnectManagerTest() = default;

TEST_F(PreconnectManagerTest, TestStartOneUrlPreresolve) {
  GURL main_frame_url("http://google.com");
  GURL url_to_preresolve("http://cdn.google.com");

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(url_to_preresolve, _))
      .WillOnce(Return(net::OK));
  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url));
  preconnect_manager_->Start(main_frame_url,
                             {PreconnectRequest(url_to_preresolve, 0)});
  // Wait for PreconnectFinished task posted to the UI thread.
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestStartOneUrlPreconnect) {
  GURL main_frame_url("http://google.com");
  GURL url_to_preconnect("http://cdn.google.com");

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(url_to_preconnect, _))
      .WillOnce(Return(net::OK));
  EXPECT_CALL(*preconnect_manager_,
              PreconnectUrl(url_to_preconnect, main_frame_url, 1, true));
  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url));
  preconnect_manager_->Start(main_frame_url,
                             {PreconnectRequest(url_to_preconnect, 1)});
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestStopOneUrlBeforePreconnect) {
  GURL main_frame_url("http://google.com");
  GURL url_to_preconnect("http://cdn.google.com");
  net::CompletionCallback callback;

  // Preconnect job isn't started before preresolve is completed asynchronously.
  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(url_to_preconnect, _))
      .WillOnce(DoAll(SaveArg<1>(&callback), Return(net::ERR_IO_PENDING)));
  preconnect_manager_->Start(main_frame_url,
                             {PreconnectRequest(url_to_preconnect, 1)});

  // Stop all jobs for |main_frame_url| before we get the callback.
  preconnect_manager_->Stop(main_frame_url);
  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url));
  callback.Run(net::OK);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestGetCallbackAfterDestruction) {
  GURL main_frame_url("http://google.com");
  GURL url_to_preconnect("http://cdn.google.com");
  net::CompletionCallback callback;
  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(url_to_preconnect, _))
      .WillOnce(DoAll(SaveArg<1>(&callback), Return(net::ERR_IO_PENDING)));
  preconnect_manager_->Start(main_frame_url,
                             {PreconnectRequest(url_to_preconnect, 1)});

  // Callback may outlive PreconnectManager but it shouldn't cause a crash.
  preconnect_manager_ = nullptr;
  callback.Run(net::OK);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestUnqueuedPreresolvesCanceled) {
  GURL main_frame_url("http://google.com");
  size_t count = PreconnectManager::kMaxInflightPreresolves;
  std::vector<PreconnectRequest> requests;
  // Allocate the space for callbacks at once because we need stable pointers.
  std::vector<net::CompletionCallback> callbacks(count);
  for (size_t i = 0; i < count; ++i) {
    // Exactly PreconnectManager::kMaxInflightPreresolves should be preresolved.
    requests.emplace_back(
        GURL(base::StringPrintf("http://cdn%" PRIuS ".google.com", i)), 1);
    EXPECT_CALL(*preconnect_manager_, PreresolveUrl(requests.back().origin, _))
        .WillOnce(
            DoAll(SaveArg<1>(&callbacks[i]), Return(net::ERR_IO_PENDING)));
  }
  // This url shouldn't be preresolved.
  requests.emplace_back(GURL("http://no.preresolve.com"), 1);
  preconnect_manager_->Start(main_frame_url, std::move(requests));

  preconnect_manager_->Stop(main_frame_url);
  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url));
  for (auto& callback : callbacks)
    callback.Run(net::OK);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestTwoConcurrentMainFrameUrls) {
  GURL main_frame_url1("http://google.com");
  GURL url_to_preconnect1("http://cdn.google.com");
  net::CompletionCallback callback1;
  GURL main_frame_url2("http://facebook.com");
  GURL url_to_preconnect2("http://cdn.facebook.com");
  net::CompletionCallback callback2;

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(url_to_preconnect1, _))
      .WillOnce(DoAll(SaveArg<1>(&callback1), Return(net::ERR_IO_PENDING)));
  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(url_to_preconnect2, _))
      .WillOnce(DoAll(SaveArg<1>(&callback2), Return(net::ERR_IO_PENDING)));
  preconnect_manager_->Start(main_frame_url1,
                             {PreconnectRequest(url_to_preconnect1, 1)});
  preconnect_manager_->Start(main_frame_url2,
                             {PreconnectRequest(url_to_preconnect2, 1)});
  // Check that the first url didn't block the second one.
  Mock::VerifyAndClearExpectations(preconnect_manager_.get());

  preconnect_manager_->Stop(main_frame_url2);
  // Stopping the second url shouldn't stop the first one.
  EXPECT_CALL(*preconnect_manager_,
              PreconnectUrl(url_to_preconnect1, main_frame_url1, 1, true));
  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url1));
  callback1.Run(net::OK);
  // No preconnect for the second url.
  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url2));
  callback2.Run(net::OK);
  base::RunLoop().RunUntilIdle();
}

// Checks that the PreconnectManager handles no more than one URL per host
// simultaneously.
TEST_F(PreconnectManagerTest, TestTwoConcurrentSameHostMainFrameUrls) {
  GURL main_frame_url1("http://google.com/search?query=cats");
  GURL url_to_preconnect1("http://cats.google.com");
  net::CompletionCallback callback1;
  GURL main_frame_url2("http://google.com/search?query=dogs");
  GURL url_to_preconnect2("http://dogs.google.com");

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(url_to_preconnect1, _))
      .WillOnce(DoAll(SaveArg<1>(&callback1), Return(net::ERR_IO_PENDING)));
  preconnect_manager_->Start(main_frame_url1,
                             {PreconnectRequest(url_to_preconnect1, 1)});
  // This suggestion should be dropped because the PreconnectManager already has
  // a job for the "google.com" host.
  preconnect_manager_->Start(main_frame_url2,
                             {PreconnectRequest(url_to_preconnect2, 1)});

  EXPECT_CALL(*preconnect_manager_,
              PreconnectUrl(url_to_preconnect1, main_frame_url1, 1, true));
  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url1));
  callback1.Run(net::OK);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestStartPreresolveHost) {
  GURL url("http://cdn.google.com/script.js");
  GURL origin("http://cdn.google.com");

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(origin, _))
      .WillOnce(Return(net::OK));
  preconnect_manager_->StartPreresolveHost(url);
  // PreconnectFinished shouldn't be called.
  base::RunLoop().RunUntilIdle();

  // Non http url shouldn't be preresovled.
  GURL non_http_url("file:///tmp/index.html");
  preconnect_manager_->StartPreresolveHost(non_http_url);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestStartPreresolveHosts) {
  GURL cdn("http://cdn.google.com");
  GURL fonts("http://fonts.google.com");

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(cdn, _))
      .WillOnce(Return(net::OK));
  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(fonts, _))
      .WillOnce(Return(net::OK));
  preconnect_manager_->StartPreresolveHosts({cdn.host(), fonts.host()});
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestStartPreconnectUrl) {
  GURL url("http://cdn.google.com/script.js");
  GURL origin("http://cdn.google.com");
  bool allow_credentials = false;

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(origin, _))
      .WillOnce(Return(net::OK));
  EXPECT_CALL(*preconnect_manager_,
              PreconnectUrl(origin, GURL(), 1, allow_credentials));
  preconnect_manager_->StartPreconnectUrl(url, allow_credentials);
  base::RunLoop().RunUntilIdle();

  // Non http url shouldn't be preconnected.
  GURL non_http_url("file:///tmp/index.html");
  preconnect_manager_->StartPreconnectUrl(non_http_url, allow_credentials);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestDetachedRequestHasHigherPriority) {
  GURL main_frame_url("http://google.com");
  size_t count = PreconnectManager::kMaxInflightPreresolves;
  std::vector<PreconnectRequest> requests;
  std::vector<net::CompletionCallback> callbacks(count);
  // Create enough asynchronous jobs to leave the last one in the queue.
  for (size_t i = 0; i < count; ++i) {
    requests.emplace_back(
        GURL(base::StringPrintf("http://cdn%" PRIuS ".google.com", i)), 0);
    EXPECT_CALL(*preconnect_manager_, PreresolveUrl(requests.back().origin, _))
        .WillOnce(
            DoAll(SaveArg<1>(&callbacks[i]), Return(net::ERR_IO_PENDING)));
  }
  // This url will wait in the queue.
  GURL queued_url("http://fonts.google.com");
  requests.emplace_back(queued_url, 0);
  preconnect_manager_->Start(main_frame_url, std::move(requests));

  // This url should come to the front of the queue.
  GURL detached_preresolve("http://ads.google.com");
  preconnect_manager_->StartPreresolveHost(detached_preresolve);
  Mock::VerifyAndClearExpectations(preconnect_manager_.get());

  net::CompletionCallback detached_callback;
  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(detached_preresolve, _))
      .WillOnce(
          DoAll(SaveArg<1>(&detached_callback), Return(net::ERR_IO_PENDING)));
  callbacks[0].Run(net::OK);

  Mock::VerifyAndClearExpectations(preconnect_manager_.get());
  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(queued_url, _))
      .WillOnce(Return(net::OK));
  detached_callback.Run(net::OK);

  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url));
  for (size_t i = 1; i < count; ++i)
    callbacks[i].Run(net::OK);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestHSTSRedirectRespectedForPreconnect) {
  net::TransportSecurityState transport_security_state;
  transport_security_state.AddHSTS(
      "google.com", base::Time::Now() + base::TimeDelta::FromDays(1000), false);
  context_getter_->GetURLRequestContext()->set_transport_security_state(
      &transport_security_state);

  GURL url("http://google.com/search");
  bool allow_credentials = false;

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(GURL("http://google.com"), _))
      .WillOnce(Return(net::OK));
  EXPECT_CALL(
      *preconnect_manager_,
      PreconnectUrl(GURL("https://google.com"), GURL(), 1, allow_credentials));
  preconnect_manager_->StartPreconnectUrl(url, allow_credentials);
  base::RunLoop().RunUntilIdle();
}

class MockProxyConfigService : public net::ProxyConfigService {
 public:
  explicit MockProxyConfigService(const net::ProxyConfig& config)
      : config_(net::ProxyConfigWithAnnotation(config,
                                               TRAFFIC_ANNOTATION_FOR_TESTS)) {}
  void AddObserver(Observer* observer) override {}
  void RemoveObserver(Observer* observer) override {}
  ConfigAvailability GetLatestProxyConfig(
      net::ProxyConfigWithAnnotation* results) override {
    *results = config_;
    return CONFIG_VALID;
  }

 private:
  net::ProxyConfigWithAnnotation config_;
};

// Tests that the predictor doesn't preresolve in the presence of the proxy
// server.
TEST_F(PreconnectManagerTest, TestPreresolveSkippedIfProxyEnabled) {
  net::ProxyConfig proxy_config;
  proxy_config.proxy_rules().ParseFromString("foopy:8080");
  proxy_config.set_auto_detect(false);
  net::ProxyResolutionService proxy_service(
      std::make_unique<MockProxyConfigService>(proxy_config),
      std::make_unique<net::MockAsyncProxyResolverFactory>(false), nullptr);
  context_getter_->GetURLRequestContext()->set_proxy_resolution_service(
      &proxy_service);

  GURL main_frame_url("http://google.com");
  GURL url_to_preconnect("http://cdn.google.com");
  GURL url_to_preresolve("http://images.google.com");

  EXPECT_CALL(*preconnect_manager_,
              PreconnectUrl(url_to_preconnect, main_frame_url, 1, true));
  EXPECT_CALL(*mock_delegate_, PreconnectFinishedProxy(main_frame_url));
  preconnect_manager_->Start(main_frame_url,
                             {PreconnectRequest(url_to_preconnect, 1),
                              PreconnectRequest(url_to_preresolve, 0)});
  base::RunLoop().RunUntilIdle();
}

}  // namespace predictors
