// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/chromium_url_request.h"

#include <memory>

#include "base/callback_helpers.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"

namespace remoting {

ChromiumUrlRequest::ChromiumUrlRequest(
    scoped_refptr<net::URLRequestContextGetter> url_context,
    UrlRequest::Type type,
    const std::string& url,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  net::URLFetcher::RequestType request_type = net::URLFetcher::GET;
  switch (type) {
    case Type::GET:
      request_type = net::URLFetcher::GET;
      break;
    case Type::POST:
      request_type = net::URLFetcher::POST;
      break;
  }
  url_fetcher_ = net::URLFetcher::Create(GURL(url), request_type, this,
                                         traffic_annotation);
  url_fetcher_->SetRequestContext(url_context.get());
  url_fetcher_->SetReferrer("https://chrome.google.com/remotedesktop");
  url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES);
}

ChromiumUrlRequest::~ChromiumUrlRequest() = default;

void ChromiumUrlRequest::AddHeader(const std::string& value) {
  url_fetcher_->AddExtraRequestHeader(value);
}

void ChromiumUrlRequest::SetPostData(const std::string& content_type,
                                     const std::string& data) {
  url_fetcher_->SetUploadData(content_type, data);
}

void ChromiumUrlRequest::Start(const OnResultCallback& on_result_callback) {
  DCHECK(!on_result_callback.is_null());
  DCHECK(on_result_callback_.is_null());

  on_result_callback_ = on_result_callback;
  url_fetcher_->Start();
}

void ChromiumUrlRequest::OnURLFetchComplete(
    const net::URLFetcher* url_fetcher) {
  DCHECK_EQ(url_fetcher, url_fetcher_.get());

  Result result;
  result.success =
      url_fetcher_->GetResponseCode() != net::URLFetcher::RESPONSE_CODE_INVALID;
  if (result.success) {
    result.status = url_fetcher_->GetResponseCode();
    url_fetcher_->GetResponseAsString(&result.response_body);
  }

  DCHECK(!on_result_callback_.is_null());
  base::ResetAndReturn(&on_result_callback_).Run(result);
}

ChromiumUrlRequestFactory::ChromiumUrlRequestFactory(
    scoped_refptr<net::URLRequestContextGetter> url_context)
    : url_context_(url_context) {}
ChromiumUrlRequestFactory::~ChromiumUrlRequestFactory() = default;

std::unique_ptr<UrlRequest> ChromiumUrlRequestFactory::CreateUrlRequest(
    UrlRequest::Type type,
    const std::string& url,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  return std::make_unique<ChromiumUrlRequest>(url_context_, type, url,
                                              traffic_annotation);
}

}  // namespace remoting
