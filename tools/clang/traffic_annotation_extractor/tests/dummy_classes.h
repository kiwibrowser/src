// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "net/traffic_annotation/network_traffic_annotation.h"

// This file provides all required dummy classes for:
// tools/clang/traffic_annotation_extractor/tests/test-original.cc

class GURL {};

namespace net {

class URLRequest {
 public:
  class Delegate;
};

class URLFetcherDelegate;

enum RequestPriority { TEST_VALUE };

class URLFetcher {
 public:
  enum RequestType { TEST_VALUE };

  static std::unique_ptr<URLFetcher> Create(
      const GURL& url,
      URLFetcher::RequestType request_type,
      URLFetcherDelegate* d);

  static std::unique_ptr<URLFetcher> Create(
      int id,
      const GURL& url,
      URLFetcher::RequestType request_type,
      URLFetcherDelegate* d);

  static std::unique_ptr<URLFetcher> Create(
      const GURL& url,
      URLFetcher::RequestType request_type,
      URLFetcherDelegate* d,
      NetworkTrafficAnnotationTag traffic_annotation);

  static std::unique_ptr<URLFetcher> Create(
      int id,
      const GURL& url,
      URLFetcher::RequestType request_type,
      URLFetcherDelegate* d,
      NetworkTrafficAnnotationTag traffic_annotation);
};

class URLRequestContext {
 public:
  std::unique_ptr<URLRequest> CreateRequest(
      const GURL& url,
      RequestPriority priority,
      URLRequest::Delegate* delegate) const;

  std::unique_ptr<URLRequest> CreateRequest(
      const GURL& url,
      RequestPriority priority,
      URLRequest::Delegate* delegate,
      NetworkTrafficAnnotationTag traffic_annotation) const;
};

}  // namespace net