// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_GENERIC_URL_REQUEST_JOB_H_
#define HEADLESS_PUBLIC_UTIL_GENERIC_URL_REQUEST_JOB_H_

#include <stddef.h>
#include <functional>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "headless/public/headless_export.h"
#include "headless/public/util/managed_dispatch_url_request_job.h"
#include "headless/public/util/url_fetcher.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace net {
class IOBuffer;
class UploadElementReader;
}  // namespace net

namespace content {
class ResourceRequestInfo;
}  // namespace content

namespace headless {

class HeadlessBrowserContext;
class URLRequestDispatcher;

// Wrapper around net::URLRequest with helpers to access select metadata.
class HEADLESS_EXPORT Request {
 public:
  virtual uint64_t GetRequestId() const = 0;

  // Deprecated, use more specific getters below instead.
  virtual const net::URLRequest* GetURLRequest() const = 0;

  virtual const std::string& GetMethod() const = 0;

  virtual const GURL& GetURL() const = 0;

  // Larger numbers mean higher priority.
  virtual int GetPriority() const = 0;

  virtual const net::HttpRequestHeaders& GetHttpRequestHeaders() const = 0;

  // The frame from which the request came from.
  virtual std::string GetDevToolsFrameId() const = 0;

  // The devtools agent host id for the page where the request came from.
  virtual std::string GetDevToolsAgentHostId() const = 0;

  // Gets the POST data, if any, from the net::URLRequest.
  virtual std::string GetPostData() const = 0;

  // Returns the size of the POST data, if any, from the net::URLRequest.
  virtual uint64_t GetPostDataSize() const = 0;

  // Returns true if the fetch was issues by the browser.
  virtual bool IsBrowserSideFetch() const = 0;

  enum class ResourceType {
    MAIN_FRAME = 0,
    SUB_FRAME = 1,
    STYLESHEET = 2,
    SCRIPT = 3,
    IMAGE = 4,
    FONT_RESOURCE = 5,
    SUB_RESOURCE = 6,
    OBJECT = 7,
    MEDIA = 8,
    WORKER = 9,
    SHARED_WORKER = 10,
    PREFETCH = 11,
    FAVICON = 12,
    XHR = 13,
    PING = 14,
    SERVICE_WORKER = 15,
    CSP_REPORT = 16,
    PLUGIN_RESOURCE = 17,
    LAST_TYPE
  };

  virtual ResourceType GetResourceType() const = 0;

  // Whether or not an asynchronous IPC was used to load this resource.
  virtual bool IsAsync() const = 0;

  virtual bool HasUserGesture() const = 0;

 protected:
  Request() {}
  virtual ~Request() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Request);
};

// Intended for use in a protocol handler, this ManagedDispatchURLRequestJob has
// the following features:
//
// 1. The delegate can extension observe / cancel and redirect requests
// 2. The delegate can optionally provide the results, otherwise the specifed
//    fetcher is invoked.
class HEADLESS_EXPORT GenericURLRequestJob
    : public ManagedDispatchURLRequestJob,
      public URLFetcher::ResultListener,
      public Request {
 public:
  class HEADLESS_EXPORT Delegate {
   public:
    // Notifies the delegate of any fetch failure. Called on an arbitrary
    // thread.
    virtual void OnResourceLoadFailed(const Request* request,
                                      net::Error error) = 0;

    // Signals that a resource load has finished. Called on an arbitrary thread.
    virtual void OnResourceLoadComplete(
        const Request* request,
        const GURL& final_url,
        scoped_refptr<net::HttpResponseHeaders> response_headers,
        const char* body,
        size_t body_size) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // NOTE |url_request_dispatcher| and |delegate| must outlive the
  // GenericURLRequestJob.
  GenericURLRequestJob(net::URLRequest* request,
                       net::NetworkDelegate* network_delegate,
                       URLRequestDispatcher* url_request_dispatcher,
                       std::unique_ptr<URLFetcher> url_fetcher,
                       Delegate* delegate,
                       HeadlessBrowserContext* headless_browser_context);
  ~GenericURLRequestJob() override;

  // net::URLRequestJob implementation:
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  void Start() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  bool GetMimeType(std::string* mime_type) const override;
  bool GetCharset(std::string* charset) override;
  void GetLoadTimingInfo(net::LoadTimingInfo* load_timing_info) const override;
  int64_t GetTotalReceivedBytes() const override;

  // URLFetcher::ResultListener implementation:
  void OnFetchStartError(net::Error error) override;
  void OnFetchComplete(const GURL& final_url,
                       scoped_refptr<net::HttpResponseHeaders> response_headers,
                       const char* body,
                       size_t body_size,
                       scoped_refptr<net::IOBufferWithSize> metadata,
                       const net::LoadTimingInfo& load_timing_info,
                       size_t total_received_bytes) override;

 protected:
  // Request implementation:
  uint64_t GetRequestId() const override;
  const net::HttpRequestHeaders& GetHttpRequestHeaders() const override;
  const net::URLRequest* GetURLRequest() const override;
  const std::string& GetMethod() const override;
  const GURL& GetURL() const override;
  int GetPriority() const override;
  std::string GetDevToolsFrameId() const override;
  std::string GetDevToolsAgentHostId() const override;
  std::string GetPostData() const override;
  uint64_t GetPostDataSize() const override;
  ResourceType GetResourceType() const override;
  bool IsAsync() const override;
  bool HasUserGesture() const override;
  bool IsBrowserSideFetch() const override;

 private:
  void PrepareCookies(const GURL& rewritten_url,
                      const std::string& method,
                      const url::Origin& site_for_cookies);
  void OnCookiesAvailable(const GURL& rewritten_url,
                          const std::string& method,
                          const net::CookieList& cookie_list);

  const std::vector<std::unique_ptr<net::UploadElementReader>>*
  GetInitializedReaders() const;

  std::unique_ptr<URLFetcher> url_fetcher_;
  net::HttpRequestHeaders extra_request_headers_;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;
  scoped_refptr<base::SingleThreadTaskRunner> origin_task_runner_;
  Delegate* delegate_;          // Not owned.
  HeadlessBrowserContext* headless_browser_context_;           // Not owned.
  const content::ResourceRequestInfo* request_resource_info_;  // Not owned.
  const char* body_ = nullptr;  // Not owned.
  scoped_refptr<net::IOBufferWithSize> metadata_;
  size_t body_size_ = 0;
  size_t read_offset_ = 0;
  net::LoadTimingInfo load_timing_info_;
  size_t total_received_bytes_ = 0;

  base::WeakPtrFactory<GenericURLRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GenericURLRequestJob);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_GENERIC_URL_REQUEST_JOB_H_
