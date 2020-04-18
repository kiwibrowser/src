// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/site_affiliation/asset_link_retriever.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"

namespace password_manager {

AssetLinkRetriever::AssetLinkRetriever(GURL file_url)
    : url_(std::move(file_url)), state_(State::INACTIVE), error_(false) {
  DCHECK(url_.is_valid());
  DCHECK(url_.SchemeIs(url::kHttpsScheme));
}

void AssetLinkRetriever::Start(net::URLRequestContextGetter* context_getter) {
  if (state_ != State::INACTIVE)
    return;
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("asset_links", R"(
        semantics {
          sender: "Asset Links Fetcher"
          description:
            "The asset links is a JSON file hosted on "
            "https://<domain>/.well-known/assetlinks.json. It contains "
            "different permissions the site gives to apps/other sites. Chrome "
            "looks for a permission to delegate the credentials from the site "
            "to another domain. It's used for handling the stored credentials "
            "in the password manager."
          trigger:
            "Load a site where it's possible to sign-in. The site can have a "
            "password form or use the Credential Management API."
          data: "None."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting: "No setting"
          policy_exception_justification:
            "The file is considered to be a resource of the page loaded."
        })");
  fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this,
                                     traffic_annotation);
  fetcher_->SetRequestContext(context_getter);
  fetcher_->SetLoadFlags(
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_DO_NOT_SEND_AUTH_DATA | net::LOAD_MAYBE_USER_GESTURE);
  fetcher_->SetStopOnRedirect(true);
  fetcher_->Start();
  state_ = State::NETWORK_REQUEST;
}

AssetLinkRetriever::~AssetLinkRetriever() = default;

void AssetLinkRetriever::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(source == fetcher_.get());

  error_ = !source->GetStatus().is_success() ||
           source->GetResponseCode() != net::HTTP_OK;
  if (error_) {
    state_ = State::FINISHED;
  } else {
    state_ = State::PARSING;
    std::string response_string;
    source->GetResponseAsString(&response_string);
    scoped_refptr<base::TaskRunner> task_runner =
        base::CreateTaskRunnerWithTraits({base::TaskPriority::USER_BLOCKING});
    auto data = std::make_unique<AssetLinkData>();
    AssetLinkData* data_raw = data.get();
    base::PostTaskAndReplyWithResult(
        task_runner.get(), FROM_HERE,
        base::BindOnce(&AssetLinkData::Parse, base::Unretained(data_raw),
                       std::move(response_string)),
        base::BindOnce(&AssetLinkRetriever::OnResponseParsed, this,
                       std::move(data)));
  }
  fetcher_.reset();
}

void AssetLinkRetriever::OnResponseParsed(std::unique_ptr<AssetLinkData> data,
                                          bool result) {
  error_ = !result;
  if (result)
    data_ = std::move(*data);
  state_ = State::FINISHED;
}

}  // namespace password_manager
