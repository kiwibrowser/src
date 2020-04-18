// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_REQUEST_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_REQUEST_H_

#include "base/logging.h"
#include "base/sequence_checker.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace net {
class URLRequest;
}

namespace network {
struct ResourceRequest;
}

namespace content {
class AppCacheURLLoaderRequest;
class AppCacheURLRequest;

// Interface for an AppCache request. Subclasses implement this interface to
// wrap custom request objects like URLRequest, etc to ensure that these
// dependencies stay out of the AppCache code.
class CONTENT_EXPORT AppCacheRequest {
 public:
  virtual ~AppCacheRequest();

  // The URL for this request.
  virtual const GURL& GetURL() const = 0;

  // The method for this request
  virtual const std::string& GetMethod() const = 0;

  // Used for cookie policy.
  virtual const GURL& GetSiteForCookies() const = 0;

  // The referrer for this request.
  virtual const GURL GetReferrer() const = 0;

  // Returns true if the request was successful.
  virtual bool IsSuccess() const = 0;

  // Returns true if the request was cancelled.
  virtual bool IsCancelled() const = 0;

  // Returns true if the request had an error.
  virtual bool IsError() const = 0;

  // Returns the HTTP response code.
  virtual int GetResponseCode() const = 0;

  // Get response header(s) by name. Returns an empty string if the header
  // wasn't found,
  virtual std::string GetResponseHeaderByName(
      const std::string& name) const = 0;

  // Returns true if the scheme and method are supported for AppCache.
  static bool IsSchemeAndMethodSupportedForAppCache(
      const AppCacheRequest* request);

  // Returns the underlying AppCacheURLRequest if any. This only applies to
  // AppCache requests loaded via the URLRequest mechanism
  virtual AppCacheURLRequest* AsURLRequest();

  // Returns the underlying AppCacheURLLoaderRequest if any. This only applies
  // to AppCache requests loaded via the URLLoader mechanism.
  virtual AppCacheURLLoaderRequest* AsURLLoaderRequest();

 protected:
  friend class AppCacheRequestHandler;
  // Enables the AppCacheJob to call GetURLRequest() and GetResourceRequest().
  friend class AppCacheJob;

  AppCacheRequest() {}

  // Getters for the request types we currently support.
  virtual net::URLRequest* GetURLRequest();

  // Returns the underlying ResourceRequest. Please note that only one of
  // GetURLRequest() and GetResourceRequest() should return valid results.
  virtual network::ResourceRequest* GetResourceRequest();

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(AppCacheRequest);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_REQUEST_H_
