// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_URL_REQUEST_JOB_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_URL_REQUEST_JOB_H_

#include <stdint.h>

#include <string>

#include "base/callback.h"
#include "content/browser/appcache/appcache_entry.h"
#include "content/browser/appcache/appcache_job.h"
#include "content/browser/appcache/appcache_storage.h"
#include "content/common/content_export.h"
#include "net/url_request/url_request_job.h"

namespace net {
class GrowableIOBuffer;
};

namespace content {
class AppCacheHost;
class AppCacheRequestHandlerTest;
namespace appcache_url_request_job_unittest {
class AppCacheURLRequestJobTest;
}

// A net::URLRequestJob derivative that knows how to return a response stored
// in the appcache.
class CONTENT_EXPORT AppCacheURLRequestJob : public AppCacheJob,
                                             public AppCacheStorage::Delegate,
                                             public net::URLRequestJob {
 public:
  ~AppCacheURLRequestJob() override;

  // AppCacheJob overrides.
  bool IsStarted() const override;
  void DeliverAppCachedResponse(const GURL& manifest_url,
                                int64_t cache_id,
                                const AppCacheEntry& entry,
                                bool is_fallback) override;
  void DeliverNetworkResponse() override;
  void DeliverErrorResponse() override;
  AppCacheURLRequestJob* AsURLRequestJob() override;
  base::WeakPtr<AppCacheJob> GetWeakPtr() override;
  base::WeakPtr<AppCacheURLRequestJob> GetDerivedWeakPtr();

  // Accessors for the info about the appcached response, if any,
  // that this job has been instructed to deliver. These are only
  // valid to call if is_delivering_appcache_response.
  const GURL& manifest_url() const { return manifest_url_; }
  int64_t cache_id() const { return cache_id_; }
  const AppCacheEntry& entry() const { return entry_; }

  // Returns true if the job has been killed.
  bool has_been_killed() const {
    return has_been_killed_;
  }

 private:
  friend class AppCacheRequestHandlerTest;
  friend class appcache_url_request_job_unittest::AppCacheURLRequestJobTest;
  // AppCacheRequestHandler::CreateJob() creates this instance.
  friend class AppCacheRequestHandler;

  // Callback that will be invoked before the request is restarted. The caller
  // can use this opportunity to grab state from the AppCacheURLRequestJob to
  // determine how it should behave when the request is restarted.
  using OnPrepareToRestartCallback = base::OnceClosure;

  AppCacheURLRequestJob(net::URLRequest* request,
                        net::NetworkDelegate* network_delegate,
                        AppCacheStorage* storage,
                        AppCacheHost* host,
                        bool is_main_resource,
                        OnPrepareToRestartCallback restart_callback_);

  // Returns true if one of the Deliver methods has been called.
  bool has_delivery_orders() const { return !IsWaiting(); }

  void MaybeBeginDelivery();
  void BeginDelivery();
  void BeginErrorDelivery(const char* message);

  // AppCacheStorage::Delegate methods
  void OnResponseInfoLoaded(AppCacheResponseInfo* response_info,
                            int64_t response_id) override;

  const net::HttpResponseInfo* http_info() const;

  // AppCacheResponseReader completion callback
  void OnReadComplete(int result);

  // net::URLRequestJob methods, see url_request_job.h for doc comments
  void Start() override;
  void Kill() override;
  net::LoadState GetLoadState() const override;
  bool GetCharset(std::string* charset) override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  net::HostPortPair GetSocketAddress() const override;

  // Sets extra request headers for Job types that support request headers.
  // This is how we get informed of range-requests.
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;

  // FilterContext methods
  bool GetMimeType(std::string* mime_type) const override;

  // Invokes |prepare_to_restart_callback_| and then calls
  // net::URLRequestJob::NotifyRestartRequired.
  void NotifyRestartRequired();

  AppCacheHost* host_;
  AppCacheStorage* storage_;
  base::TimeTicks start_time_tick_;
  bool has_been_started_;
  bool has_been_killed_;
  GURL manifest_url_;
  int64_t cache_id_;
  AppCacheEntry entry_;
  bool is_fallback_;
  bool is_main_resource_;  // Used for histogram logging.
  scoped_refptr<net::GrowableIOBuffer> handler_source_buffer_;
  std::unique_ptr<AppCacheResponseReader> handler_source_reader_;
  scoped_refptr<AppCache> cache_;
  scoped_refptr<AppCacheGroup> group_;
  OnPrepareToRestartCallback on_prepare_to_restart_callback_;
  base::WeakPtrFactory<AppCacheURLRequestJob> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AppCacheURLRequestJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_URL_REQUEST_JOB_H_
