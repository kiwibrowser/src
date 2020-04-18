// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/guest_view/web_view/web_ui/web_ui_url_fetcher.h"

#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/url_fetcher.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"

WebUIURLFetcher::WebUIURLFetcher(content::BrowserContext* context,
                                 int render_process_id,
                                 int render_frame_id,
                                 const GURL& url,
                                 WebUILoadFileCallback callback)
    : context_(context),
      render_process_id_(render_process_id),
      render_frame_id_(render_frame_id),
      url_(url),
      callback_(std::move(callback)) {}

WebUIURLFetcher::~WebUIURLFetcher() {
}

void WebUIURLFetcher::Start() {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("webui_content_scripts_download", R"(
        semantics {
          sender: "WebView"
          description:
            "When a WebView is embedded within a WebUI, it needs to fetch the "
            "embedder's content scripts from Chromium's network stack for its "
            "content scripts injection API."
          trigger: "The content script injection API is called."
          data: "URL of the script file to be downloaded."
          destination: LOCAL
        }
        policy {
          cookies_allowed: NO
          setting: "It is not possible to disable this feature from settings."
          policy_exception_justification:
            "Not Implemented, considered not useful as the request doesn't "
            "go to the network."
        })");
  fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this,
                                     traffic_annotation);
  fetcher_->SetRequestContext(
      content::BrowserContext::GetDefaultStoragePartition(context_)->
          GetURLRequestContext());
  fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                         net::LOAD_DO_NOT_SEND_COOKIES);

  content::AssociateURLFetcherWithRenderFrame(
      fetcher_.get(), url::Origin::Create(url_), render_process_id_,
      render_frame_id_);
  fetcher_->Start();
}

void WebUIURLFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  CHECK_EQ(fetcher_.get(), source);

  std::unique_ptr<std::string> data(new std::string());
  bool result = false;
  if (fetcher_->GetStatus().status() == net::URLRequestStatus::SUCCESS) {
    result = fetcher_->GetResponseAsString(data.get());
    DCHECK(result);
  }
  fetcher_.reset();
  std::move(callback_).Run(result, std::move(data));
}
