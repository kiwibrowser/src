// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_manifest_downloader.h"

#include <unordered_map>
#include <utility>

#include "base/logging.h"
#include "base/optional.h"
#include "base/stl_util.h"
#include "base/strings/string_split.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/link_header_util/link_header_util.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace payments {
namespace {

GURL ParseResponseHeader(const net::URLFetcher* source) {
  if (source->GetResponseCode() != net::HTTP_OK &&
      source->GetResponseCode() != net::HTTP_NO_CONTENT) {
    LOG(ERROR) << "Unable to make a HEAD request to " << source->GetURL()
               << " for payment method manifest.";
    return GURL();
  }

  net::HttpResponseHeaders* headers = source->GetResponseHeaders();
  if (!headers) {
    LOG(ERROR) << "No HTTP headers found on " << source->GetURL()
               << " for payment method manifest.";
    return GURL();
  }

  std::string link_header;
  headers->GetNormalizedHeader("link", &link_header);
  if (link_header.empty()) {
    LOG(ERROR) << "No HTTP Link headers found on " << source->GetURL()
               << " for payment method manifest.";
    return GURL();
  }

  for (const auto& value : link_header_util::SplitLinkHeader(link_header)) {
    std::string payment_method_manifest_url;
    std::unordered_map<std::string, base::Optional<std::string>> params;
    if (!link_header_util::ParseLinkHeaderValue(
            value.first, value.second, &payment_method_manifest_url, &params)) {
      continue;
    }

    auto rel = params.find("rel");
    if (rel == params.end())
      continue;

    std::vector<std::string> rel_parts =
        base::SplitString(rel->second.value_or(""), HTTP_LWS,
                          base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (base::ContainsValue(rel_parts, "payment-method-manifest"))
      return source->GetOriginalURL().Resolve(payment_method_manifest_url);
  }

  LOG(ERROR) << "No rel=\"payment-method-manifest\" HTTP Link headers found on "
             << source->GetURL() << " for payment method manifest.";
  return GURL();
}

bool IsValidManifestUrl(const GURL& url) {
  return url.is_valid() &&
         (url.SchemeIs(url::kHttpsScheme) ||
          (url.SchemeIs(url::kHttpScheme) && net::IsLocalhost(url)));
}

GURL ParseRedirectUrlFromResponseHeader(const net::URLFetcher* source) {
  // Do not follow net::HTTP_MULTIPLE_CHOICES, net::HTTP_NOT_MODIFIED and
  // net::HTTP_USE_PROXY redirects.
  if (source->GetResponseCode() != net::HTTP_MOVED_PERMANENTLY &&
      source->GetResponseCode() != net::HTTP_FOUND &&
      source->GetResponseCode() != net::HTTP_SEE_OTHER &&
      source->GetResponseCode() != net::HTTP_TEMPORARY_REDIRECT &&
      source->GetResponseCode() != net::HTTP_PERMANENT_REDIRECT) {
    return GURL();
  }

  if (!IsValidManifestUrl(source->GetURL()))
    return GURL();

  return source->GetURL();
}

std::string ParseResponseContent(const net::URLFetcher* source) {
  std::string content;
  if (source->GetResponseCode() != net::HTTP_OK) {
    LOG(ERROR) << "Unable to download " << source->GetURL()
               << " for payment manifests.";
    return content;
  }

  bool success = source->GetResponseAsString(&content);
  DCHECK(success);  // Whether the fetcher was set to store result as string.

  return content;
}

}  // namespace

PaymentManifestDownloader::PaymentManifestDownloader(
    const scoped_refptr<net::URLRequestContextGetter>& context)
    : context_(context) {}

PaymentManifestDownloader::~PaymentManifestDownloader() {}

void PaymentManifestDownloader::DownloadPaymentMethodManifest(
    const GURL& url,
    PaymentManifestDownloadCallback callback) {
  DCHECK(IsValidManifestUrl(url));
  // Restrict number of redirects for efficiency and breaking circle.
  InitiateDownload(url, net::URLFetcher::HEAD,
                   /*allowed_number_of_redirects=*/3, std::move(callback));
}

void PaymentManifestDownloader::DownloadWebAppManifest(
    const GURL& url,
    PaymentManifestDownloadCallback callback) {
  DCHECK(IsValidManifestUrl(url));
  InitiateDownload(url, net::URLFetcher::GET, /*allowed_number_of_redirects=*/0,
                   std::move(callback));
}

PaymentManifestDownloader::Download::Download() {}

PaymentManifestDownloader::Download::~Download() {}

void PaymentManifestDownloader::OnURLFetchComplete(
    const net::URLFetcher* source) {
  auto download_it = downloads_.find(source);
  DCHECK(download_it != downloads_.end());

  std::unique_ptr<Download> download = std::move(download_it->second);
  downloads_.erase(download_it);

  if (download->request_type == net::URLFetcher::HEAD) {
    // Manually follow some type of redirects.
    if (download->allowed_number_of_redirects > 0) {
      GURL redirect_url = ParseRedirectUrlFromResponseHeader(source);
      if (!redirect_url.is_empty()) {
        InitiateDownload(redirect_url, net::URLFetcher::HEAD,
                         --download->allowed_number_of_redirects,
                         std::move(download->callback));
        return;
      }
    }

    GURL url = ParseResponseHeader(source);
    if (IsValidManifestUrl(url)) {
      InitiateDownload(url, net::URLFetcher::GET,
                       /*allowed_number_of_redirects=*/0,
                       std::move(download->callback));
    } else {
      // If the URL is empty, then ParseResponseHeader() has already printed
      // an explanation.
      if (!url.is_empty())
        LOG(ERROR) << url << " is not a valid payment method manifest URL.";
      std::move(download->callback).Run(std::string());
    }
  } else {
    std::move(download->callback).Run(ParseResponseContent(source));
  }
}

void PaymentManifestDownloader::InitiateDownload(
    const GURL& url,
    net::URLFetcher::RequestType request_type,
    int allowed_number_of_redirects,
    PaymentManifestDownloadCallback callback) {
  DCHECK(IsValidManifestUrl(url));

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("payment_manifest_downloader", R"(
        semantics {
          sender: "Web Payments"
          description:
            "Chromium downloads manifest files for web payments API to help "
            "users make secure and convenient payments on the web."
          trigger:
            "A user that has a payment app visits a website that uses the web "
            "payments API."
          data: "None."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled in settings. Users can uninstall/"
            "disable all payment apps to stop this feature."
          policy_exception_justification: "Not implemented."
        })");
  std::unique_ptr<net::URLFetcher> fetcher = net::URLFetcher::Create(
      0 /* id */, url, request_type, this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher.get(), data_use_measurement::DataUseUserData::PAYMENTS);
  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SAVE_COOKIES);
  fetcher->SetStopOnRedirect(true);
  fetcher->SetRequestContext(context_.get());
  fetcher->Start();

  auto download = std::make_unique<Download>();
  download->request_type = request_type;
  download->fetcher = std::move(fetcher);
  download->callback = std::move(callback);
  download->allowed_number_of_redirects = allowed_number_of_redirects;

  const net::URLFetcher* identifier = download->fetcher.get();
  auto insert_result =
      downloads_.insert(std::make_pair(identifier, std::move(download)));
  DCHECK(insert_result.second);  // Whether the insert has succeeded.
}

}  // namespace payments
