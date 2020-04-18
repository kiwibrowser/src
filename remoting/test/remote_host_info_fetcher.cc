// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/remote_host_info_fetcher.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_fetcher.h"
#include "remoting/base/url_request_context_getter.h"

namespace {
const char kRequestTestOrigin[] =
    "Origin: chrome-extension://ljacajndfccfgnfohlgkdphmbnpkjflk";
}

namespace remoting {
namespace test {

RemoteHostInfoFetcher::RemoteHostInfoFetcher() = default;

RemoteHostInfoFetcher::~RemoteHostInfoFetcher() = default;

bool RemoteHostInfoFetcher::RetrieveRemoteHostInfo(
    const std::string& application_id,
    const std::string& access_token,
    ServiceEnvironment service_environment,
    const RemoteHostInfoCallback& callback) {
  DCHECK(!application_id.empty());
  DCHECK(!access_token.empty());
  DCHECK(!callback.is_null());
  DCHECK(remote_host_info_callback_.is_null());

  VLOG(2) << "RemoteHostInfoFetcher::RetrieveRemoteHostInfo() called";

  std::string service_url(
      GetRunApplicationUrl(application_id, service_environment));
  if (service_url.empty()) {
    LOG(ERROR) << "Unrecognized service type: " << service_environment;
    return false;
  }
  VLOG(1) << "Using remote host service request url: " << service_url;

  remote_host_info_callback_ = callback;

  request_context_getter_ = new remoting::URLRequestContextGetter(
      base::ThreadTaskRunnerHandle::Get());

  request_ = net::URLFetcher::Create(GURL(service_url), net::URLFetcher::POST,
                                     this, TRAFFIC_ANNOTATION_FOR_TESTS);
  request_->SetRequestContext(request_context_getter_.get());
  request_->AddExtraRequestHeader("Authorization: OAuth " + access_token);
  request_->AddExtraRequestHeader(kRequestTestOrigin);
  request_->SetUploadData("application/json; charset=UTF-8", "{}");
  request_->Start();

  return true;
}

void RemoteHostInfoFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(source);
  VLOG(2) << "URL Fetch Completed for: " << source->GetOriginalURL();

  RemoteHostInfo remote_host_info;
  int response_code = request_->GetResponseCode();
  if (response_code != net::HTTP_OK) {
    LOG(ERROR) << "RemoteHostInfo request failed with error code: "
               << response_code;
    base::ResetAndReturn(&remote_host_info_callback_).Run(remote_host_info);
    return;
  }

  std::string response_string;
  if (!request_->GetResponseAsString(&response_string)) {
    LOG(ERROR) << "Failed to retrieve RemoteHostInfo response data";
    base::ResetAndReturn(&remote_host_info_callback_).Run(remote_host_info);
    return;
  }

  std::unique_ptr<base::Value> response_value(
      base::JSONReader::Read(response_string));
  if (!response_value || !response_value->is_dict()) {
    LOG(ERROR) << "Failed to parse response string to JSON";
    base::ResetAndReturn(&remote_host_info_callback_).Run(remote_host_info);
    return;
  }

  std::string remote_host_status;
  const base::DictionaryValue* response;
  if (response_value->GetAsDictionary(&response)) {
    response->GetString("status", &remote_host_status);
  } else {
    LOG(ERROR) << "Failed to convert parsed JSON to a dictionary object";
    base::ResetAndReturn(&remote_host_info_callback_).Run(remote_host_info);
    return;
  }

  remote_host_info.SetRemoteHostStatusFromString(remote_host_status);

  if (remote_host_info.IsReadyForConnection()) {
    response->GetString("host.applicationId", &remote_host_info.application_id);
    response->GetString("host.hostId", &remote_host_info.host_id);
    response->GetString("hostJid", &remote_host_info.host_jid);
    response->GetString("authorizationCode",
                        &remote_host_info.authorization_code);
    response->GetString("sharedSecret", &remote_host_info.shared_secret);
  }

  base::ResetAndReturn(&remote_host_info_callback_).Run(remote_host_info);
}

}  // namespace test
}  // namespace remoting
