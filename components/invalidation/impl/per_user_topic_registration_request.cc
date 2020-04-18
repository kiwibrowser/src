// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/per_user_topic_registration_request.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"

using net::HttpRequestHeaders;
using net::URLRequestStatus;

namespace {

const char kPrivateTopicNameKey[] = "private_topic_name";

base::Value* GetPrivateTopicName(base::Value* value) {
  if (!value || !value->is_dict()) {
    return nullptr;
  }
  return value->FindKeyOfType(kPrivateTopicNameKey, base::Value::Type::STRING);
}

};  // namespace

namespace syncer {

PerUserTopicRegistrationRequest::PerUserTopicRegistrationRequest()
    : weak_ptr_factory_(this) {}

PerUserTopicRegistrationRequest::~PerUserTopicRegistrationRequest() = default;

void PerUserTopicRegistrationRequest::Start(
    CompletedCallback callback,
    ParseJSONCallback parse_json,
    network::mojom::URLLoaderFactory* loader_factory) {
  DCHECK(request_completed_callback_.is_null()) << "Request already running!";
  request_completed_callback_ = std::move(callback);
  parse_json_ = std::move(parse_json);
  simple_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory,
      base::BindOnce(&PerUserTopicRegistrationRequest::OnURLFetchComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PerUserTopicRegistrationRequest::OnURLFetchComplete(
    std::unique_ptr<std::string> response_body) {
  int response_code = 0;
  if (simple_loader_->ResponseInfo() &&
      simple_loader_->ResponseInfo()->headers) {
    response_code = simple_loader_->ResponseInfo()->headers->response_code();
  }
  OnURLFetchCompleteInternal(simple_loader_->NetError(), response_code,
                             std::move(response_body));
}

void PerUserTopicRegistrationRequest::OnURLFetchCompleteInternal(
    int net_error,
    int response_code,
    std::unique_ptr<std::string> response_body) {
  if (net_error != net::OK) {
    std::move(request_completed_callback_)
        .Run(Status(StatusCode::FAILED, base::StringPrintf("Network Error")),
             std::string());
    return;
  }

  if (response_code != net::HTTP_OK) {
    std::move(request_completed_callback_)
        .Run(Status(StatusCode::FAILED,
                    base::StringPrintf("HTTP Error: %d", response_code)),
             std::string());
    return;
  }

  if (!response_body || response_body->empty()) {
    std::move(request_completed_callback_)
        .Run(Status(StatusCode::FAILED, base::StringPrintf("Body parse error")),
             std::string());
    return;
  }
  std::move(parse_json_)
      .Run(*response_body,
           base::BindOnce(&PerUserTopicRegistrationRequest::OnJsonParseSuccess,
                          weak_ptr_factory_.GetWeakPtr()),
           base::BindOnce(&PerUserTopicRegistrationRequest::OnJsonParseFailure,
                          weak_ptr_factory_.GetWeakPtr()));
}

void PerUserTopicRegistrationRequest::OnJsonParseFailure(
    const std::string& error) {
  std::move(request_completed_callback_)
      .Run(Status(StatusCode::FAILED, base::StringPrintf("Body parse error")),
           std::string());
}

void PerUserTopicRegistrationRequest::OnJsonParseSuccess(
    std::unique_ptr<base::Value> value) {
  const base::Value* private_topic_name_value =
      GetPrivateTopicName(value.get());
  if (private_topic_name_value) {
    std::move(request_completed_callback_)
        .Run(Status(StatusCode::SUCCESS, std::string()),
             private_topic_name_value->GetString());
  } else {
    std::move(request_completed_callback_)
        .Run(Status(StatusCode::FAILED, base::StringPrintf("Body parse error")),
             std::string());
  }
}

PerUserTopicRegistrationRequest::Builder::Builder() = default;
PerUserTopicRegistrationRequest::Builder::Builder(
    PerUserTopicRegistrationRequest::Builder&&) = default;
PerUserTopicRegistrationRequest::Builder::~Builder() = default;

std::unique_ptr<PerUserTopicRegistrationRequest>
PerUserTopicRegistrationRequest::Builder::Build() const {
  DCHECK(!scope_.empty());
  auto request = base::WrapUnique(new PerUserTopicRegistrationRequest);

  GURL full_url(base::StringPrintf(
      "%s/v1/perusertopics/%s/rel/topics/?subscriber_token=%s", scope_.c_str(),
      project_id_.c_str(), token_.c_str()));

  DCHECK(full_url.is_valid());

  request->url_ = full_url;

  std::string body = BuildBody();
  net::HttpRequestHeaders headers = BuildHeaders();
  request->simple_loader_ = BuildURLFetcher(headers, body, full_url);

  // Log the request for debugging network issues.
  DVLOG(1) << "Building a subscription request to " << full_url << ":\n"
           << headers.ToString() << "\n"
           << body;
  return request;
}

PerUserTopicRegistrationRequest::Builder&
PerUserTopicRegistrationRequest::Builder::SetToken(const std::string& token) {
  token_ = token;
  return *this;
}

PerUserTopicRegistrationRequest::Builder&
PerUserTopicRegistrationRequest::Builder::SetScope(const std::string& scope) {
  scope_ = scope;
  return *this;
}

PerUserTopicRegistrationRequest::Builder&
PerUserTopicRegistrationRequest::Builder::SetAuthenticationHeader(
    const std::string& auth_header) {
  auth_header_ = auth_header;
  return *this;
}

PerUserTopicRegistrationRequest::Builder&
PerUserTopicRegistrationRequest::Builder::SetPublicTopicName(
    const std::string& topic) {
  topic_ = topic;
  return *this;
}

PerUserTopicRegistrationRequest::Builder&
PerUserTopicRegistrationRequest::Builder::SetProjectId(
    const std::string& project_id) {
  project_id_ = project_id;
  return *this;
}

HttpRequestHeaders PerUserTopicRegistrationRequest::Builder::BuildHeaders()
    const {
  HttpRequestHeaders headers;
  if (!auth_header_.empty()) {
    headers.SetHeader(HttpRequestHeaders::kAuthorization, auth_header_);
  }
  return headers;
}

std::string PerUserTopicRegistrationRequest::Builder::BuildBody() const {
  base::DictionaryValue request;

  request.SetString("public_topic_name", topic_);

  std::string request_json;
  bool success = base::JSONWriter::Write(request, &request_json);
  DCHECK(success);
  return request_json;
}

std::unique_ptr<network::SimpleURLLoader>
PerUserTopicRegistrationRequest::Builder::BuildURLFetcher(
    const HttpRequestHeaders& headers,
    const std::string& body,
    const GURL& url) const {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("per_user_topic_registration_request",
                                          R"(
        semantics {
          sender: "Register the Sync client for listening of the specific topic"
          description:
            "Chromium can receive Sync invalidations via FCM messages."
            "This request registers the client for receiving messages for the"
            "concrete topic. In case of Chrome Sync topic is a ModelType,"
            "e.g. BOOKMARK"
          trigger:
            "Subscription takes place only once per profile per topic. "
          data:
            "An OAuth2 token is sent as an authorization header."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature can not be disabled by settings now"
          chrome_policy: {
             SyncDisabled {
               policy_options {mode: MANDATORY}
               SyncDisabled: false
             }
          }
        })");

  auto request = std::make_unique<network::ResourceRequest>();
  request->method = "POST";
  request->url = url;
  request->headers = headers;

  std::unique_ptr<network::SimpleURLLoader> url_loader =
      network::SimpleURLLoader::Create(std::move(request), traffic_annotation);
  url_loader->AttachStringForUpload(body, "application/json; charset=UTF-8");

  return url_loader;
}

}  // namespace syncer
