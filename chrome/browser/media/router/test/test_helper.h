// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_TEST_TEST_HELPER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_TEST_TEST_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/media/router/issue_manager.h"
#include "chrome/browser/media/router/issues_observer.h"
#include "chrome/browser/media/router/media_routes_observer.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "content/public/common/presentation_connection_message.h"
#include "testing/gmock/include/gmock/gmock.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service.h"
#include "chrome/browser/media/router/discovery/dial/dial_url_fetcher.h"
#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service.h"
#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl.h"
#include "chrome/browser/media/router/providers/cast/cast_app_discovery_service.h"
#include "chrome/browser/media/router/providers/dial/dial_activity_manager.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "net/base/ip_endpoint.h"
#include "services/network/test/test_url_loader_factory.h"
#endif  // !defined(OS_ANDROID)

namespace media_router {

// Matcher for objects that uses Equals() member function for equality check.
MATCHER_P(Equals, other, "") {
  return arg.Equals(other);
}

// Matcher for a sequence of objects that uses Equals() member function for
// equality check.
MATCHER_P(SequenceEquals, other, "") {
  if (arg.size() != other.size()) {
    return false;
  }
  for (size_t i = 0; i < arg.size(); ++i) {
    if (!arg[i].Equals(other[i])) {
      return false;
    }
  }
  return true;
}

// Matcher for IssueInfo title.
MATCHER_P(IssueTitleEquals, title, "") {
  return arg.info().title == title;
}

MATCHER_P(StateChangeInfoEquals, other, "") {
  return arg.state == other.state && arg.close_reason == other.close_reason &&
         arg.message == other.message;
}

std::string PresentationConnectionMessageToString(
    const content::PresentationConnectionMessage& message);

class MockIssuesObserver : public IssuesObserver {
 public:
  explicit MockIssuesObserver(IssueManager* issue_manager);
  ~MockIssuesObserver() override;

  MOCK_METHOD1(OnIssue, void(const Issue& issue));
  MOCK_METHOD0(OnIssuesCleared, void());
};

class MockMediaSinksObserver : public MediaSinksObserver {
 public:
  MockMediaSinksObserver(MediaRouter* router,
                         const MediaSource& source,
                         const url::Origin& origin);
  ~MockMediaSinksObserver() override;

  MOCK_METHOD1(OnSinksReceived, void(const std::vector<MediaSink>& sinks));
};

class MockMediaRoutesObserver : public MediaRoutesObserver {
 public:
  explicit MockMediaRoutesObserver(
      MediaRouter* router,
      const MediaSource::Id source_id = std::string());
  ~MockMediaRoutesObserver() override;

  MOCK_METHOD2(OnRoutesUpdated,
               void(const std::vector<MediaRoute>& routes,
                    const std::vector<MediaRoute::Id>& joinable_route_ids));
};

class MockPresentationConnectionProxy
    : public blink::mojom::PresentationConnection {
 public:
  // PresentationConnectionMessage is move-only.
  // TODO(crbug.com/729950): Use MOCK_METHOD directly once GMock gets the
  // move-only type support.
  MockPresentationConnectionProxy();
  ~MockPresentationConnectionProxy() override;
  void OnMessage(content::PresentationConnectionMessage message,
                 OnMessageCallback cb) override {
    OnMessageInternal(message, cb);
  }
  MOCK_METHOD2(OnMessageInternal,
               void(const content::PresentationConnectionMessage&,
                    OnMessageCallback&));
  MOCK_METHOD1(DidChangeState,
               void(blink::mojom::PresentationConnectionState state));
  MOCK_METHOD0(RequestClose, void());
};

#if !defined(OS_ANDROID)
class MockDialMediaSinkService : public DialMediaSinkService {
 public:
  MockDialMediaSinkService();
  ~MockDialMediaSinkService() override;

  MOCK_METHOD1(Start, void(const OnSinksDiscoveredCallback&));
  MOCK_METHOD0(OnUserGesture, void());
};

class MockCastMediaSinkService : public CastMediaSinkService {
 public:
  MockCastMediaSinkService();
  ~MockCastMediaSinkService() override;

  MOCK_METHOD2(Start,
               void(const OnSinksDiscoveredCallback&, MediaSinkServiceBase*));
  MOCK_METHOD0(OnUserGesture, void());
  MOCK_METHOD0(StartMdnsDiscovery, void());
};

class MockCastAppDiscoveryService : public CastAppDiscoveryService {
 public:
  MockCastAppDiscoveryService();
  ~MockCastAppDiscoveryService() override;

  Subscription StartObservingMediaSinks(
      const CastMediaSource& source,
      const SinkQueryCallback& callback) override;
  MOCK_METHOD1(DoStartObservingMediaSinks, void(const CastMediaSource&));
  MOCK_METHOD0(Refresh, void());

  SinkQueryCallbackList& callbacks() { return callbacks_; }

 private:
  SinkQueryCallbackList callbacks_;
};

class MockDialAppDiscoveryService : public DialAppDiscoveryService {
 public:
  MockDialAppDiscoveryService();
  ~MockDialAppDiscoveryService() override;

  void FetchDialAppInfo(const MediaSinkInternal& sink,
                        const std::string& app_name,
                        DialAppInfoCallback app_info_cb) override;
  MOCK_METHOD2(DoFetchDialAppInfo,
               void(const MediaSink::Id& sink_id, const std::string& app_name));

  DialAppInfoCallback PassCallback();

 private:
  DialAppInfoCallback app_info_cb_;
};

class TestDialURLFetcher : public DialURLFetcher {
 public:
  TestDialURLFetcher(SuccessCallback success_cb,
                     ErrorCallback error_cb,
                     network::TestURLLoaderFactory* factory);
  ~TestDialURLFetcher() override;
  void Start(const GURL& url,
             const std::string& method,
             const base::Optional<std::string>& post_data,
             int max_retries) override;
  MOCK_METHOD4(DoStart,
               void(const GURL&,
                    const std::string&,
                    const base::Optional<std::string>&,
                    int));
  void StartDownload() override;

 private:
  network::TestURLLoaderFactory* const factory_;
};

class TestDialActivityManager : public DialActivityManager {
 public:
  explicit TestDialActivityManager(network::TestURLLoaderFactory* factory);
  ~TestDialActivityManager() override;

  std::unique_ptr<DialURLFetcher> CreateFetcher(
      DialURLFetcher::SuccessCallback success_cb,
      DialURLFetcher::ErrorCallback error_cb) override;

  void SetExpectedRequest(const GURL& url,
                          const std::string& method,
                          const base::Optional<std::string>& post_data);

  MOCK_METHOD0(OnFetcherCreated, void());

 private:
  network::TestURLLoaderFactory* const factory_;

  GURL expected_url_;
  std::string expected_method_;
  base::Optional<std::string> expected_post_data_;

  DISALLOW_COPY_AND_ASSIGN(TestDialActivityManager);
};

// Helper function to create an IP endpoint object.
// If |num| is 1, returns 192.168.0.101:8009;
// If |num| is 2, returns 192.168.0.102:8009.
net::IPEndPoint CreateIPEndPoint(int num);

// Helper function to create a DIAL media sink object.
// If |num| is 1, returns a media sink object with following data:
// {
//   id: "id 1",
//   name: "friendly name 1",
//   extra_data {
//     model_name: "model name 1"
//     ip_address: 192.168.1.101,
//     app_url: "http://192.168.0.101/apps"
//   }
// }
MediaSinkInternal CreateDialSink(int num);

// Helper function to create a Cast sink.
MediaSinkInternal CreateCastSink(int num);

// Creates a minimal ParsedDialAppInfo with given values.
ParsedDialAppInfo CreateParsedDialAppInfo(const std::string& name,
                                          DialAppState app_state);

std::unique_ptr<ParsedDialAppInfo> CreateParsedDialAppInfoPtr(
    const std::string& name,
    DialAppState app_state);
#endif  // !defined(OS_ANDROID)

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_TEST_TEST_HELPER_H_
