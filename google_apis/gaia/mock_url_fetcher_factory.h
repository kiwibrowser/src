// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A collection of classes that are useful when testing things that use a
// GaiaAuthFetcher.

#ifndef GOOGLE_APIS_GAIA_MOCK_URL_FETCHER_FACTORY_H_
#define GOOGLE_APIS_GAIA_MOCK_URL_FETCHER_FACTORY_H_

#include <string>

#include "base/macros.h"
#include "google_apis/gaia/gaia_auth_fetcher.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"

// Responds as though ClientLogin returned from the server.
class MockFetcher : public net::TestURLFetcher {
 public:
  MockFetcher(bool success,
              const GURL& url,
              const std::string& results,
              net::URLFetcher::RequestType request_type,
              net::URLFetcherDelegate* d);

  MockFetcher(const GURL& url,
              const net::URLRequestStatus& status,
              int response_code,
              const std::string& results,
              net::URLFetcher::RequestType request_type,
              net::URLFetcherDelegate* d);

  ~MockFetcher() override;

  void Start() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockFetcher);
};

template<typename T>
class MockURLFetcherFactory : public net::URLFetcherFactory,
                              public net::ScopedURLFetcherFactory {
 public:
  MockURLFetcherFactory()
      : net::ScopedURLFetcherFactory(this),
        success_(true) {
  }
  ~MockURLFetcherFactory() {}
  std::unique_ptr<net::URLFetcher> CreateURLFetcher(
      int id,
      const GURL& url,
      net::URLFetcher::RequestType request_type,
      net::URLFetcherDelegate* d,
      net::NetworkTrafficAnnotationTag traffic_annotation) override {
    return std::unique_ptr<net::URLFetcher>(
        new T(success_, url, results_, request_type, d));
  }
  void set_success(bool success) {
    success_ = success;
  }
  void set_results(const std::string& results) {
    results_ = results;
  }
 private:
  bool success_;
  std::string results_;
  DISALLOW_COPY_AND_ASSIGN(MockURLFetcherFactory);
};

#endif  // GOOGLE_APIS_GAIA_MOCK_URL_FETCHER_FACTORY_H_
