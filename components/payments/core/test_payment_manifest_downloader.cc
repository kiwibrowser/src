// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/test_payment_manifest_downloader.h"

#include <utility>

#include "base/memory/ref_counted.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace payments {

TestDownloader::TestDownloader(
    const scoped_refptr<net::URLRequestContextGetter>& context)
    : PaymentManifestDownloader(context) {}

TestDownloader::~TestDownloader() {}

void TestDownloader::AddTestServerURL(const std::string& prefix,
                                      const GURL& test_server_url) {
  test_server_url_[prefix] = test_server_url;
}

void TestDownloader::DownloadPaymentMethodManifest(
    const GURL& url,
    PaymentManifestDownloadCallback callback) {
  GURL actual_url = url;

  // Find the first key in |test_server_url_| that is a prefix of |url|. If
  // found, then replace this prefix in the |url| with the URL of the test
  // server that should be used instead.
  for (const auto& prefix_and_url : test_server_url_) {
    const std::string& prefix = prefix_and_url.first;
    const GURL& test_server_url = prefix_and_url.second;
    if (base::StartsWith(url.spec(), prefix, base::CompareCase::SENSITIVE)) {
      actual_url =
          GURL(test_server_url.spec() + url.spec().substr(prefix.length()));
      break;
    }
  }

  PaymentManifestDownloader::DownloadPaymentMethodManifest(actual_url,
                                                           std::move(callback));
}

}  // namespace payments
