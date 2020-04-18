// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_NET_HTTP_BRIDGE_H_
#define COMPONENTS_SYNC_ENGINE_NET_HTTP_BRIDGE_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "components/sync/base/cancelation_observer.h"
#include "components/sync/engine/net/http_post_provider_factory.h"
#include "components/sync/engine/net/http_post_provider_interface.h"
#include "components/sync/engine/net/network_time_update_callback.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

class HttpBridgeTest;

namespace net {
class HttpResponseHeaders;
class URLFetcher;
}

namespace syncer {

class CancelationSignal;

// A bridge between the syncer and Chromium HTTP layers.
// Provides a way for the sync backend to use Chromium directly for HTTP
// requests rather than depending on a third party provider (e.g libcurl).
// This is a one-time use bridge. Create one for each request you want to make.
// It is RefCountedThreadSafe because it can PostTask to the io loop, and thus
// needs to stick around across context switches, etc.
class HttpBridge : public base::RefCountedThreadSafe<HttpBridge>,
                   public HttpPostProviderInterface,
                   public net::URLFetcherDelegate {
 public:
  HttpBridge(const std::string& user_agent,
             const scoped_refptr<net::URLRequestContextGetter>& context,
             const NetworkTimeUpdateCallback& network_time_update_callback,
             const BindToTrackerCallback& bind_to_tracker_callback);

  // HttpPostProviderInterface implementation.
  void SetExtraRequestHeaders(const char* headers) override;
  void SetURL(const char* url, int port) override;
  void SetPostPayload(const char* content_type,
                      int content_length,
                      const char* content) override;
  bool MakeSynchronousPost(int* error_code, int* response_code) override;
  void Abort() override;

  // WARNING: these response content methods are used to extract plain old data
  // and not null terminated strings, so you should make sure you have read
  // GetResponseContentLength() characters when using GetResponseContent. e.g
  // string r(b->GetResponseContent(), b->GetResponseContentLength()).
  int GetResponseContentLength() const override;
  const char* GetResponseContent() const override;
  const std::string GetResponseHeaderValue(
      const std::string& name) const override;

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;
  void OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                  int64_t current,
                                  int64_t total,
                                  int64_t current_network_bytes) override;
  void OnURLFetchUploadProgress(const net::URLFetcher* source,
                                int64_t current,
                                int64_t total) override;

  net::URLRequestContextGetter* GetRequestContextGetterForTest() const;

 protected:
  ~HttpBridge() override;

  // Protected virtual so the unit test can override to shunt network requests.
  virtual void MakeAsynchronousPost();

 private:
  friend class base::RefCountedThreadSafe<HttpBridge>;
  friend class SyncHttpBridgeTest;
  friend class ::HttpBridgeTest;

  // Called on the IO loop to issue the network request. The extra level
  // of indirection is so that the unit test can override this behavior but we
  // still have a function to statically pass to PostTask.
  void CallMakeAsynchronousPost() { MakeAsynchronousPost(); }

  // Used to destroy a fetcher when the bridge is Abort()ed, to ensure that
  // a reference to |this| is held while flushing any pending fetch completion
  // callbacks coming from the IO thread en route to finally destroying the
  // fetcher.
  void DestroyURLFetcherOnIOThread(net::URLFetcher* fetcher,
                                   base::Timer* fetch_timer);

  void UpdateNetworkTime();

  // Helper method to abort the request if we timed out.
  void OnURLFetchTimedOut();

  // Used to check whether a method runs on the thread that we were created on.
  // This is the thread that will block on MakeSynchronousPost while the IO
  // thread fetches data from the network.
  // This should be the main syncer thread (SyncerThread) which is what blocks
  // on network IO through curl_easy_perform.
  base::ThreadChecker thread_checker_;

  // The user agent for all requests.
  const std::string user_agent_;

  // The URL to POST to.
  GURL url_for_request_;

  // POST payload information.
  std::string content_type_;
  std::string request_content_;
  std::string extra_headers_;

  // A waitable event we use to provide blocking semantics to
  // MakeSynchronousPost. We block created_on_loop_ while the IO loop fetches
  // network request.
  base::WaitableEvent http_post_completed_;

  struct URLFetchState {
    URLFetchState();
    ~URLFetchState();
    // Our hook into the network layer is a URLFetcher. USED ONLY ON THE IO
    // LOOP, so we can block created_on_loop_ while the fetch is in progress.
    // NOTE: This is not a unique_ptr for a reason. It must be deleted on the
    // same thread that created it, which isn't the same thread |this| gets
    // deleted on. We must manually delete url_poster_ on the IO loop.
    net::URLFetcher* url_poster;

    // Start and finish time of request. Set immediately before sending
    // request and after receiving response.
    base::Time start_time;
    base::Time end_time;

    // Used to support 'Abort' functionality.
    bool aborted;

    // Cached response data.
    bool request_completed;
    bool request_succeeded;
    int http_response_code;
    int error_code;
    std::string response_content;
    scoped_refptr<net::HttpResponseHeaders> response_headers;

    // Timer to ensure http requests aren't stalled. Reset every time upload or
    // download progress is made.
    std::unique_ptr<base::Timer> http_request_timeout_timer;
  };

  // This lock synchronizes use of state involved in the flow to fetch a URL
  // using URLFetcher, including |fetch_state_| and |request_context_getter_| on
  // any thread, for example, this flow needs to be synchronized to gracefully
  // clean up URLFetcher and return appropriate values in |error_code|.
  mutable base::Lock fetch_state_lock_;
  URLFetchState fetch_state_;

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  const scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;

  // Callback for updating network time.
  NetworkTimeUpdateCallback network_time_update_callback_;

  // A callback to tag Sync request to be able to record data use of this
  // service by data_use_measurement component.
  BindToTrackerCallback bind_to_tracker_callback_;

  DISALLOW_COPY_AND_ASSIGN(HttpBridge);
};

class HttpBridgeFactory : public HttpPostProviderFactory,
                          public CancelationObserver {
 public:
  HttpBridgeFactory(
      const scoped_refptr<net::URLRequestContextGetter>&
          baseline_context_getter,
      const NetworkTimeUpdateCallback& network_time_update_callback,
      CancelationSignal* cancelation_signal);
  ~HttpBridgeFactory() override;

  // HttpPostProviderFactory:
  void Init(const std::string& user_agent,
            const BindToTrackerCallback& bind_to_tracker_callback) override;
  HttpPostProviderInterface* Create() override;
  void Destroy(HttpPostProviderInterface* http) override;

  // CancelationObserver implementation:
  void OnSignalReceived() override;

 private:
  // The user agent to use in all requests.
  std::string user_agent_;

  // Protects |request_context_getter_| to allow releasing it's reference from
  // the sync thread, even when it's in use on the IO thread.
  base::Lock request_context_getter_lock_;

  // The request context getter used for making all requests.
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  NetworkTimeUpdateCallback network_time_update_callback_;

  CancelationSignal* const cancelation_signal_;

  // A callback to tag Sync request to be able to record data use of this
  // service by data_use_measurement component.
  BindToTrackerCallback bind_to_tracker_callback_;

  DISALLOW_COPY_AND_ASSIGN(HttpBridgeFactory);
};

}  //  namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_NET_HTTP_BRIDGE_H_
