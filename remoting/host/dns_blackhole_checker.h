// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_DNS_BLACKHOLE_CHECKER_H_
#define REMOTING_HOST_DNS_BLACKHOLE_CHECKER_H_

#include "net/url_request/url_fetcher_delegate.h"

#include "base/callback.h"
#include "base/macros.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace remoting {

// This is the default prefix that is prepended to the kTalkGadgetUrl to form
// the complete talkgadget URL used by the host. Policy settings allow admins
// to change the prefix that is used.
extern const char kDefaultHostTalkGadgetPrefix[];

class DnsBlackholeChecker : public net::URLFetcherDelegate {
 public:
  DnsBlackholeChecker(
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      std::string talkgadget_prefix);
  ~DnsBlackholeChecker() override;

  // net::URLFetcherDelegate interface.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Initiates a check the verify that the host talkgadget has not been "DNS
  // blackholed" to prevent connections. If this is called again before the
  // callback has been called, then the second call is ignored.
  void CheckForDnsBlackhole(const base::Callback<void(bool)>& callback);

 private:
  // URL request context getter to use to create the URL fetcher.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // URL fetcher used to verify access to the host talkgadget.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // The string pre-pended to '.talkgadget.google.com' to create the full
  // talkgadget domain name for the host.
  std::string talkgadget_prefix_;

  // Called with the results of the connection check.
  base::Callback<void(bool)> callback_;

  DISALLOW_COPY_AND_ASSIGN(DnsBlackholeChecker);
};

}  // namespace remoting

#endif  // REMOTING_HOST_DNS_BLACKHOLE_CHECKER_H_
