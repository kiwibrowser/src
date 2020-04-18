// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_LIBADDRESSINPUT_CHROMIUM_CHROME_METADATA_SOURCE_H_
#define THIRD_PARTY_LIBADDRESSINPUT_CHROMIUM_CHROME_METADATA_SOURCE_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/source.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace autofill {

// A class for downloading rules to let libaddressinput validate international
// addresses.
class ChromeMetadataSource : public ::i18n::addressinput::Source,
                             public net::URLFetcherDelegate {
 public:
  ChromeMetadataSource(const std::string& validation_data_url,
                       net::URLRequestContextGetter* getter);
  virtual ~ChromeMetadataSource();

  // ::i18n::addressinput::Source:
  virtual void Get(const std::string& key,
                   const Callback& downloaded) const override;

  // net::URLFetcherDelegate:
  virtual void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  struct Request {
    Request(const std::string& key,
            std::unique_ptr<net::URLFetcher> fetcher,
            const Callback& callback);

    std::string key;
    // The data that's received.
    std::string data;
    // The object that manages retrieving the data.
    std::unique_ptr<net::URLFetcher> fetcher;
    const Callback& callback;
  };

  // Non-const method actually implementing Get().
  void Download(const std::string& key, const Callback& downloaded);

  const std::string validation_data_url_;
  net::URLRequestContextGetter* const getter_;  // weak

  // Maps from active URL fetcher to request metadata.
  std::map<const net::URLFetcher*, std::unique_ptr<Request>> requests_;

  DISALLOW_COPY_AND_ASSIGN(ChromeMetadataSource);
};

}  // namespace autofill

#endif  // THIRD_PARTY_LIBADDRESSINPUT_CHROMIUM_CHROME_METADATA_SOURCE_H_
