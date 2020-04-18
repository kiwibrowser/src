// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/net/net_util.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "chrome/test/chromedriver/net/url_request_context_getter.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace {

class SyncUrlFetcher : public net::URLFetcherDelegate {
 public:
  SyncUrlFetcher(const GURL& url,
                 URLRequestContextGetter* getter,
                 std::string* response)
      : url_(url),
        getter_(getter),
        response_(response),
        event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
               base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  ~SyncUrlFetcher() override {}

  bool Fetch() {
    getter_->GetNetworkTaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(&SyncUrlFetcher::FetchOnIOThread,
                                  base::Unretained(this)));
    event_.Wait();
    return success_;
  }

  void FetchOnIOThread() {
    fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this,
                                       TRAFFIC_ANNOTATION_FOR_TESTS);
    fetcher_->SetRequestContext(getter_);
    fetcher_->Start();
  }

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    success_ = (source->GetResponseCode() == 200);
    if (success_)
      success_ = source->GetResponseAsString(response_);
    fetcher_.reset();  // Destroy the fetcher on IO thread.
    event_.Signal();
  }

 private:
  GURL url_;
  URLRequestContextGetter* getter_;
  std::string* response_;
  base::WaitableEvent event_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  bool success_;
};

}  // namespace

NetAddress::NetAddress() : port_(-1) {}

NetAddress::NetAddress(int port) : host_("localhost"), port_(port) {}

NetAddress::NetAddress(const std::string& host, int port)
    : host_(host), port_(port) {}

NetAddress::~NetAddress() {}

bool NetAddress::IsValid() const {
  return port_ >= 0 && port_ < (1 << 16);
}

std::string NetAddress::ToString() const {
  return host_ + base::StringPrintf(":%d", port_);
}

const std::string& NetAddress::host() const {
  return host_;
}

int NetAddress::port() const {
  return port_;
}

bool FetchUrl(const std::string& url,
              URLRequestContextGetter* getter,
              std::string* response) {
  return SyncUrlFetcher(GURL(url), getter, response).Fetch();
}
