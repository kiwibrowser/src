// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/test/fake_server/fake_server_http_post_provider.h"

#include "base/bind.h"
#include "base/location.h"
#include "components/sync/test/fake_server/fake_server.h"

using syncer::HttpPostProviderInterface;

namespace fake_server {

FakeServerHttpPostProviderFactory::FakeServerHttpPostProviderFactory(
    const base::WeakPtr<FakeServer>& fake_server,
    scoped_refptr<base::SequencedTaskRunner> fake_server_task_runner)
    : fake_server_(fake_server),
      fake_server_task_runner_(fake_server_task_runner) {}

FakeServerHttpPostProviderFactory::~FakeServerHttpPostProviderFactory() {}

void FakeServerHttpPostProviderFactory::Init(
    const std::string& user_agent,
    const syncer::BindToTrackerCallback& bind_to_tracker_callback) {}

HttpPostProviderInterface* FakeServerHttpPostProviderFactory::Create() {
  FakeServerHttpPostProvider* http =
      new FakeServerHttpPostProvider(fake_server_, fake_server_task_runner_);
  http->AddRef();
  return http;
}

void FakeServerHttpPostProviderFactory::Destroy(
    HttpPostProviderInterface* http) {
  static_cast<FakeServerHttpPostProvider*>(http)->Release();
}

FakeServerHttpPostProvider::FakeServerHttpPostProvider(
    const base::WeakPtr<FakeServer>& fake_server,
    scoped_refptr<base::SequencedTaskRunner> fake_server_task_runner)
    : fake_server_(fake_server),
      fake_server_task_runner_(fake_server_task_runner) {}

FakeServerHttpPostProvider::~FakeServerHttpPostProvider() {}

void FakeServerHttpPostProvider::SetExtraRequestHeaders(const char* headers) {
  // TODO(pvalenzuela): Add assertions on this value.
  extra_request_headers_.assign(headers);
}

void FakeServerHttpPostProvider::SetURL(const char* url, int port) {
  // TODO(pvalenzuela): Add assertions on these values.
  request_url_.assign(url);
  request_port_ = port;
}

void FakeServerHttpPostProvider::SetPostPayload(const char* content_type,
                                                int content_length,
                                                const char* content) {
  request_content_type_.assign(content_type);
  request_content_.assign(content, content_length);
}

bool FakeServerHttpPostProvider::MakeSynchronousPost(int* error_code,
                                                     int* response_code) {
  // It is assumed that a POST is being made to /command.
  int post_error_code = -1;
  int post_response_code = -1;
  std::string post_response;

  base::WaitableEvent post_complete(
      base::WaitableEvent::ResetPolicy::AUTOMATIC,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  base::Closure signal_closure = base::Bind(&base::WaitableEvent::Signal,
                                            base::Unretained(&post_complete));

  bool result = fake_server_task_runner_->PostTask(
      FROM_HERE, base::Bind(&FakeServer::HandleCommand, fake_server_,
                            base::ConstRef(request_content_),
                            base::ConstRef(signal_closure), &post_error_code,
                            &post_response_code, &post_response));

  if (!result)
    return false;

  post_complete.Wait();
  post_error_code_ = post_error_code;
  post_response_code_ = post_response_code;
  response_ = post_response;

  *error_code = post_error_code_;
  *response_code = post_response_code_;
  return *error_code == 0;
}

int FakeServerHttpPostProvider::GetResponseContentLength() const {
  return response_.length();
}

const char* FakeServerHttpPostProvider::GetResponseContent() const {
  return response_.c_str();
}

const std::string FakeServerHttpPostProvider::GetResponseHeaderValue(
    const std::string& name) const {
  return std::string();
}

void FakeServerHttpPostProvider::Abort() {}

}  // namespace fake_server
