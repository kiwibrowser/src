// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/common/json_response_fetcher.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/service_manager_connection.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace app_list {

const char kBadResponse[] = "Bad Web Service search response";

JSONResponseFetcher::JSONResponseFetcher(
    const Callback& callback,
    content::BrowserContext* browser_context)
    : callback_(callback),
      browser_context_(browser_context),
      weak_factory_(this) {
  DCHECK(!callback_.is_null());
}

JSONResponseFetcher::~JSONResponseFetcher() {}

void JSONResponseFetcher::Start(const GURL& query_url) {
  Stop();

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("json_response_fetcher", R"(
          semantics {
            sender: "JSON Response Fetcher"
            description:
              "Chrome OS downloads data for a web store result."
            trigger:
              "When a user initiates a web store search and views results. "
            data:
              "JSON data comprising search results. "
              "No user information is sent."
            destination: GOOGLE_OWNED_SERVICE
          }
          policy {
            cookies_allowed: NO
          })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = query_url;
  resource_request->load_flags =
      net::LOAD_DO_NOT_SAVE_COOKIES | net::LOAD_DISABLE_CACHE;
  simple_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                    traffic_annotation);
  network::mojom::URLLoaderFactory* loader_factory =
      content::BrowserContext::GetDefaultStoragePartition(browser_context_)
          ->GetURLLoaderFactoryForBrowserProcess()
          .get();
  simple_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory,
      base::BindOnce(&JSONResponseFetcher::OnSimpleLoaderComplete,
                     base::Unretained(this)));
}

void JSONResponseFetcher::Stop() {
  simple_loader_.reset();
  weak_factory_.InvalidateWeakPtrs();
}

void JSONResponseFetcher::OnJsonParseSuccess(
    std::unique_ptr<base::Value> parsed_json) {
  if (!parsed_json->is_dict()) {
    OnJsonParseError(kBadResponse);
    return;
  }

  callback_.Run(base::WrapUnique(
      static_cast<base::DictionaryValue*>(parsed_json.release())));
}

void JSONResponseFetcher::OnJsonParseError(const std::string& error) {
  callback_.Run(std::unique_ptr<base::DictionaryValue>());
}

void JSONResponseFetcher::OnSimpleLoaderComplete(
    std::unique_ptr<std::string> response_body) {
  if (!response_body) {
    OnJsonParseError(kBadResponse);
    return;
  }

  // The parser will call us back via one of the callbacks.
  data_decoder::SafeJsonParser::Parse(
      content::ServiceManagerConnection::GetForProcess()->GetConnector(),
      *response_body,
      base::Bind(&JSONResponseFetcher::OnJsonParseSuccess,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&JSONResponseFetcher::OnJsonParseError,
                 weak_factory_.GetWeakPtr()));
}

}  // namespace app_list
