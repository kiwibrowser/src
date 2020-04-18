// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_PAYMENT_MANIFEST_DOWNLOADER_H_
#define COMPONENTS_PAYMENTS_CORE_PAYMENT_MANIFEST_DOWNLOADER_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"

class GURL;

namespace net {
class URLRequestContextGetter;
}

namespace payments {

// Called on completed download of a manifest. Download failure results in empty
// contents. Failure to download the manifest can happen because of the
// following reasons:
//  - HTTP response code is not 200. (204 is also allowed for HEAD request.)
//  - HTTP GET on the manifest URL returns empty content.
//
// In the case of a payment method manifest download, can also fail when:
//  - HTTP response headers are absent.
//  - HTTP response headers do not contain Link headers.
//  - Link header does not contain rel="payment-method-manifest".
//  - Link header does not contain a valid URL.
using PaymentManifestDownloadCallback =
    base::OnceCallback<void(const std::string&)>;

// Downloader of the payment method manifest and web-app manifest based on the
// payment method name that is a URL with HTTPS scheme, e.g.,
// https://bobpay.com.
//
// The downloader does not follow redirects. A download succeeds only if all
// HTTP response codes are 200 or 204.
class PaymentManifestDownloader : public net::URLFetcherDelegate {
 public:
  // |delegate| should not be null and must outlive this object.
  explicit PaymentManifestDownloader(
      const scoped_refptr<net::URLRequestContextGetter>& context);

  ~PaymentManifestDownloader() override;

  // Download a payment method manifest via two consecutive HTTP requests:
  //
  // 1) HEAD request for the payment method name. The HTTP response header is
  //    parsed for Link header that points to the location of the payment method
  //    manifest file. Example of a relative location:
  //
  //      Link: <data/payment-manifest.json>; rel="payment-method-manifest"
  //
  //    (This is relative to the payment method URL.) Example of an absolute
  //    location:
  //
  //      Link: <https://bobpay.com/data/payment-manifest.json>;
  //      rel="payment-method-manifest"
  //
  //    The absolute location must use HTTPS scheme.
  //
  // 2) GET request for the payment method manifest file.
  //
  // |url| should be a valid URL with HTTPS scheme.
  virtual void DownloadPaymentMethodManifest(
      const GURL& url,
      PaymentManifestDownloadCallback callback);

  // Download a web app manifest via a single HTTP request:
  //
  // 1) GET request for the payment method name.
  //
  // |url| should be a valid URL with HTTPS scheme.
  virtual void DownloadWebAppManifest(const GURL& url,
                                      PaymentManifestDownloadCallback callback);

 private:
  // Information about an ongoing download request.
  struct Download {
    Download();
    ~Download();

    int allowed_number_of_redirects = 0;
    net::URLFetcher::RequestType request_type;
    std::unique_ptr<net::URLFetcher> fetcher;
    PaymentManifestDownloadCallback callback;
  };

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void InitiateDownload(const GURL& url,
                        net::URLFetcher::RequestType request_type,
                        int allowed_number_of_redirects,
                        PaymentManifestDownloadCallback callback);

  scoped_refptr<net::URLRequestContextGetter> context_;

  // Downloads are identified by net::URLFetcher pointers, because that's the
  // only unique piece of information that OnURLFetchComplete() receives. Can't
  // rely on the URL of the download, because of possible collision between HEAD
  // and GET requests.
  std::map<const net::URLFetcher*, std::unique_ptr<Download>> downloads_;

  DISALLOW_COPY_AND_ASSIGN(PaymentManifestDownloader);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_PAYMENT_MANIFEST_DOWNLOADER_H_
