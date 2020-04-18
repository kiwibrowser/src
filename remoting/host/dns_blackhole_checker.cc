// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/dns_blackhole_checker.h"

#include "base/callback_helpers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "remoting/base/logging.h"
#include "url/gurl.h"

namespace remoting {

// Default prefix added to the base talkgadget URL.
const char kDefaultHostTalkGadgetPrefix[] = "chromoting-host";

// The base talkgadget URL.
const char kTalkGadgetUrl[] = ".talkgadget.google.com/talkgadget/"
                              "oauth/chrome-remote-desktop-host";

DnsBlackholeChecker::DnsBlackholeChecker(
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    std::string talkgadget_prefix)
    : url_request_context_getter_(request_context_getter),
      talkgadget_prefix_(talkgadget_prefix) {
}

DnsBlackholeChecker::~DnsBlackholeChecker() = default;

// This is called in response to the TalkGadget http request initiated from
// CheckStatus().
void DnsBlackholeChecker::OnURLFetchComplete(const net::URLFetcher* source) {
  int response = source->GetResponseCode();
  bool allow = false;
  if (source->GetResponseCode() == 200) {
    HOST_LOG << "Successfully connected to host talkgadget.";
    allow = true;
  } else {
    HOST_LOG << "Unable to connect to host talkgadget (" << response << ")";
  }
  url_fetcher_.reset(nullptr);
  base::ResetAndReturn(&callback_).Run(allow);
}

void DnsBlackholeChecker::CheckForDnsBlackhole(
    const base::Callback<void(bool)>& callback) {
  // Make sure we're not currently in the middle of a connection check.
  if (!url_fetcher_.get()) {
    DCHECK(callback_.is_null());
    callback_ = callback;
    std::string talkgadget_url("https://");
    if (talkgadget_prefix_.empty()) {
      talkgadget_url += kDefaultHostTalkGadgetPrefix;
    } else {
      talkgadget_url += talkgadget_prefix_;
    }
    talkgadget_url += kTalkGadgetUrl;
    HOST_LOG << "Verifying connection to " << talkgadget_url;
    url_fetcher_ = net::URLFetcher::Create(GURL(talkgadget_url),
                                           net::URLFetcher::GET, this);
    url_fetcher_->SetRequestContext(url_request_context_getter_.get());
    url_fetcher_->Start();
  } else {
    HOST_LOG << "Pending connection check";
  }
}

}  // namespace remoting
