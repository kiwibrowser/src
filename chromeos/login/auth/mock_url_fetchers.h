// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_MOCK_URL_FETCHERS_H_
#define CHROMEOS_LOGIN_AUTH_MOCK_URL_FETCHERS_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/chromeos_export.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace net {
class URLFetcherDelegate;
}

namespace chromeos {

// Simulates a URL fetch by posting a delayed task. This fetch expects to be
// canceled, and fails the test if it is not
class ExpectCanceledFetcher : public net::TestURLFetcher {
 public:
  ExpectCanceledFetcher(bool success,
                        const GURL& url,
                        const std::string& results,
                        net::URLFetcher::RequestType request_type,
                        net::URLFetcherDelegate* d);
  ~ExpectCanceledFetcher() override;

  void Start() override;

  void CompleteFetch();

 private:
  base::WeakPtrFactory<ExpectCanceledFetcher> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(ExpectCanceledFetcher);
};

class GotCanceledFetcher : public net::TestURLFetcher {
 public:
  GotCanceledFetcher(bool success,
                     const GURL& url,
                     const std::string& results,
                     net::URLFetcher::RequestType request_type,
                     net::URLFetcherDelegate* d);
  ~GotCanceledFetcher() override;

  void Start() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GotCanceledFetcher);
};

class SuccessFetcher : public net::TestURLFetcher {
 public:
  SuccessFetcher(bool success,
                 const GURL& url,
                 const std::string& results,
                 net::URLFetcher::RequestType request_type,
                 net::URLFetcherDelegate* d);
  ~SuccessFetcher() override;

  void Start() override;

 private:
  void RunDelegate();

  base::WeakPtrFactory<SuccessFetcher> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(SuccessFetcher);
};

class FailFetcher : public net::TestURLFetcher {
 public:
  FailFetcher(bool success,
              const GURL& url,
              const std::string& results,
              net::URLFetcher::RequestType request_type,
              net::URLFetcherDelegate* d);
  ~FailFetcher() override;

  void Start() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FailFetcher);
};

class CaptchaFetcher : public net::TestURLFetcher {
 public:
  CaptchaFetcher(bool success,
                 const GURL& url,
                 const std::string& results,
                 net::URLFetcher::RequestType request_type,
                 net::URLFetcherDelegate* d);
  ~CaptchaFetcher() override;

  static std::string GetCaptchaToken();
  static std::string GetCaptchaUrl();
  static std::string GetUnlockUrl();

  void Start() override;

 private:
  static const char kCaptchaToken[];
  static const char kCaptchaUrlBase[];
  static const char kCaptchaUrlFragment[];
  static const char kUnlockUrl[];
  DISALLOW_COPY_AND_ASSIGN(CaptchaFetcher);
};

class HostedFetcher : public net::TestURLFetcher {
 public:
  HostedFetcher(bool success,
                const GURL& url,
                const std::string& results,
                net::URLFetcher::RequestType request_type,
                net::URLFetcherDelegate* d);
  ~HostedFetcher() override;

  void Start() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(HostedFetcher);
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_MOCK_URL_FETCHERS_H_
