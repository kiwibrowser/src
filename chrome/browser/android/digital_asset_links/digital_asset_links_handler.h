// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DIGITAL_ASSET_LINKS_DIGITAL_ASSET_LINKS_HANDLER_H_
#define CHROME_BROWSER_ANDROID_DIGITAL_ASSET_LINKS_DIGITAL_ASSET_LINKS_HANDLER_H_

#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"

namespace digital_asset_links {

extern const char kDigitalAssetLinksCheckResponseKeyLinked[];

// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.browserservices
enum class RelationshipCheckResult {
  SUCCESS = 0,
  FAILURE,
  NO_CONNECTION
};

using RelationshipCheckResultCallback =
  base::OnceCallback<void(RelationshipCheckResult)>;

// A handler class for sending REST API requests to DigitalAssetLinks web
// end point. See
// https://developers.google.com/digital-asset-links/v1/getting-started
// for details of usage and APIs. These APIs are used to verify declared
// relationships between different asset types like web domains or Android apps.
// The lifecycle of this handler will be governed by the owner.
class DigitalAssetLinksHandler : public net::URLFetcherDelegate {
 public:
  explicit DigitalAssetLinksHandler(
      const scoped_refptr<net::URLRequestContextGetter>& request_context);
  ~DigitalAssetLinksHandler() override;

  // Checks whether the given "relationship" has been declared by the target
  // |web_domain| for the source Android app which is uniquely defined by the
  // |package_name| and SHA256 |fingerprint| (a string with 32 hexadecimals
  // with : in between) given. Any error in the string params
  // here will result in a bad request and a nullptr response to the callback.
  //
  // Calling this multiple times on the same handler will cancel the previous
  // checks.
  // See
  // https://developers.google.com/digital-asset-links/reference/rest/v1/assetlinks/check
  // for details.
  bool CheckDigitalAssetLinkRelationship(
      RelationshipCheckResultCallback callback,
      const std::string& web_domain,
      const std::string& package_name,
      const std::string& fingerprint,
      const std::string& relationship);

 private:
  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Callbacks for the SafeJsonParser.
  void OnJSONParseSucceeded(std::unique_ptr<base::Value> result);
  void OnJSONParseFailed(const std::string& error_message);

  scoped_refptr<net::URLRequestContextGetter> request_context_;

  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // The per request callback for receiving a URLFetcher result. This gets
  // reset every time we get a new CheckDigitalAssetLinkRelationship call.
  RelationshipCheckResultCallback callback_;

  base::WeakPtrFactory<DigitalAssetLinksHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DigitalAssetLinksHandler);
};

}  // namespace digital_asset_links

#endif  // CHROME_BROWSER_ANDROID_DIGITAL_ASSET_LINKS_DIGITAL_ASSET_LINKS_HANDLER_H_
