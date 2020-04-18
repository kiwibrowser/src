// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/digital_asset_links/digital_asset_links_handler.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_status.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"

namespace {
const char kDigitalAssetLinksBaseURL[] =
    "https://digitalassetlinks.googleapis.com";
const char kDigitalAssetLinksCheckAPI[] = "/v1/assetlinks:check?";
const char kTargetOriginParam[] = "source.web.site";
const char kSourcePackageNameParam[] = "target.androidApp.packageName";
const char kSourceFingerprintParam[] =
    "target.androidApp.certificate.sha256Fingerprint";
const char kRelationshipParam[] = "relation";

GURL GetUrlForCheckingRelationship(const std::string& web_domain,
                                   const std::string& package_name,
                                   const std::string& fingerprint,
                                   const std::string& relationship) {
  GURL request_url =
      GURL(kDigitalAssetLinksBaseURL).Resolve(kDigitalAssetLinksCheckAPI);
  request_url =
      net::AppendQueryParameter(request_url, kTargetOriginParam, web_domain);
  request_url = net::AppendQueryParameter(request_url, kSourcePackageNameParam,
                                          package_name);
  request_url = net::AppendQueryParameter(request_url, kSourceFingerprintParam,
                                          fingerprint);
  request_url =
      net::AppendQueryParameter(request_url, kRelationshipParam, relationship);
  DCHECK(request_url.is_valid());
  return request_url;
}
}  // namespace

namespace digital_asset_links {

const char kDigitalAssetLinksCheckResponseKeyLinked[] = "linked";

DigitalAssetLinksHandler::DigitalAssetLinksHandler(
    const scoped_refptr<net::URLRequestContextGetter>& request_context)
    : request_context_(request_context), weak_ptr_factory_(this) {}

DigitalAssetLinksHandler::~DigitalAssetLinksHandler() = default;

void DigitalAssetLinksHandler::OnURLFetchComplete(
    const net::URLFetcher* source) {

  if (!source->GetStatus().is_success() ||
      source->GetResponseCode() != net::HTTP_OK) {
    if (source->GetStatus().error() == net::ERR_INTERNET_DISCONNECTED
        || source->GetStatus().error() == net::ERR_NAME_NOT_RESOLVED) {
      LOG(WARNING) << "Digital Asset Links connection failed.";
      std::move(callback_).Run(RelationshipCheckResult::NO_CONNECTION);
      return;
    }

    LOG(WARNING) << base::StringPrintf(
        "Digital Asset Links endpoint responded with code %d.",
        source->GetResponseCode());
    std::move(callback_).Run(RelationshipCheckResult::FAILURE);
    return;
  }

  std::string response_body;
  source->GetResponseAsString(&response_body);

  data_decoder::SafeJsonParser::Parse(
      /* connector=*/nullptr,  // Connector is unused on Android.
      response_body,
      base::Bind(&DigitalAssetLinksHandler::OnJSONParseSucceeded,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&DigitalAssetLinksHandler::OnJSONParseFailed,
                 weak_ptr_factory_.GetWeakPtr()));

  url_fetcher_.reset(nullptr);
}

void DigitalAssetLinksHandler::OnJSONParseSucceeded(
    std::unique_ptr<base::Value> result) {
  base::Value* success = result->FindKeyOfType(
      kDigitalAssetLinksCheckResponseKeyLinked, base::Value::Type::BOOLEAN);

  std::move(callback_).Run(success && success->GetBool()
      ? RelationshipCheckResult::SUCCESS
      : RelationshipCheckResult::FAILURE);
}

void DigitalAssetLinksHandler::OnJSONParseFailed(
    const std::string& error_message) {
  LOG(WARNING)
      << base::StringPrintf(
             "Digital Asset Links response parsing failed with message:")
      << error_message;
  std::move(callback_).Run(RelationshipCheckResult::FAILURE);
}

bool DigitalAssetLinksHandler::CheckDigitalAssetLinkRelationship(
    RelationshipCheckResultCallback callback,
    const std::string& web_domain,
    const std::string& package_name,
    const std::string& fingerprint,
    const std::string& relationship) {
  GURL request_url = GetUrlForCheckingRelationship(web_domain, package_name,
                                                   fingerprint, relationship);

  if (!request_url.is_valid())
    return false;

  // Resetting both the callback and URLFetcher here to ensure that any previous
  // requests will never get a OnUrlFetchComplete. This effectively cancels
  // any checks that was done over this handler.
  callback_ = std::move(callback);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("digital_asset_links", R"(
        semantics {
          sender: "Digital Asset Links Handler"
          description:
            "Digital Asset Links APIs allows any caller to check pre declared"
            "relationships between two assets which can be either web domains"
            "or native applications. This requests checks for a specific "
            "relationship declared by a web site with an Android application"
          trigger:
            "When the related application makes a claim to have the queried"
            "relationship with the web domain"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting: "Not user controlled. But the verification is a trusted API"
                   "that doesn't use user data"
          policy_exception_justification:
            "Not implemented, considered not useful as no content is being "
            "uploaded; this request merely downloads the resources on the web."
        })");

  url_fetcher_ = net::URLFetcher::Create(0, request_url, net::URLFetcher::GET,
                                         this, traffic_annotation);
  url_fetcher_->SetAutomaticallyRetryOn5xx(false);
  url_fetcher_->SetRequestContext(request_context_.get());
  url_fetcher_->Start();
  return true;
}

}  // namespace digital_asset_links
