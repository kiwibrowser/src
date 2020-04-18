// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_REQUEST_BASE_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_REQUEST_BASE_H_

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/optional.h"
#include "content/browser/appcache/appcache_update_job.h"
#include "net/base/io_buffer.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

class AppCacheUpdateJob::UpdateRequestBase {
 public:
  virtual ~UpdateRequestBase();

  // Creates an instance of the AppCacheUpdateRequestBase subclass.
  static std::unique_ptr<UpdateRequestBase> Create(
      AppCacheServiceImpl* appcache_service,
      const GURL& url,
      int buffer_size,
      URLFetcher* fetcher);

  // This method is called to start the request.
  virtual void Start() = 0;

  // Sets all extra request headers.  Any extra request headers set by other
  // methods are overwritten by this method.  This method may only be called
  // before Start() is called.  It is an error to call it later.
  virtual void SetExtraRequestHeaders(
      const net::HttpRequestHeaders& headers) = 0;

  // Returns the request URL.
  virtual GURL GetURL() const = 0;

  // Sets flags which control the request load. e.g. if it can be loaded
  // from cache, etc.
  virtual void SetLoadFlags(int flags) = 0;

  // Gets the load flags on the request.
  virtual int GetLoadFlags() const = 0;

  // Get the mime type.  This method may only be called after the response was
  // started.
  virtual std::string GetMimeType() const = 0;

  // Cookie policy.
  virtual void SetSiteForCookies(const GURL& site_for_cookies) = 0;

  // Sets the origin of the context which initiated the request.
  virtual void SetInitiator(const base::Optional<url::Origin>& initiator) = 0;

  // Get all response headers, as a HttpResponseHeaders object.  See comments
  // in HttpResponseHeaders class as to the format of the data.
  virtual net::HttpResponseHeaders* GetResponseHeaders() const = 0;

  // Returns the HTTP response code (e.g., 200, 404, and so on).  This method
  // may only be called once the delegate's OnResponseStarted method has been
  // called.  For non-HTTP requests, this method returns -1.
  virtual int GetResponseCode() const = 0;

  // Get the HTTP response info in its entirety.
  virtual const net::HttpResponseInfo& GetResponseInfo() const = 0;

  // Initiates an asynchronous read. Multiple concurrent reads are not
  // supported.
  virtual void Read() = 0;

  // This method may be called at any time after Start() has been called to
  // cancel the request.
  // Returns net::ERR_ABORTED or any applicable net error.
  virtual int Cancel() = 0;

 protected:
  UpdateRequestBase();

  // Returns the traffic annotation information to be used for the outgoing
  // request.
  static net::NetworkTrafficAnnotationTag GetTrafficAnnotation();
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_UPDATE_REQUEST_BASE_H_
