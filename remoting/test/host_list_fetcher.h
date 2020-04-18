// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_HOST_LIST_FETCHER_H_
#define REMOTING_TEST_HOST_LIST_FETCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "remoting/test/host_info.h"

namespace remoting {
class URLRequestContextGetter;
}

namespace remoting {
namespace test {

// Used by the HostlistFetcher to make HTTP requests and also by the
// unittests for this class to set fake response data for these URLs.
const char kHostListProdRequestUrl[] = "https://www.googleapis.com/"
    "chromoting/v1/@me/hosts";
const char kHostListTestRequestUrl[] =
    "https://www-googleapis-test.sandbox.google.com/chromoting/v1/@me/hosts";

// Requests a host list from the directory service for an access token.
// Destroying the RemoteHostInfoFetcher while a request is outstanding
// will cancel the request. It is safe to delete the fetcher from within a
// completion callback.  Must be used from a thread running a message loop.
// The public method is virtual to allow for mocking and fakes.
class HostListFetcher : public net::URLFetcherDelegate {
 public:
  HostListFetcher();
  ~HostListFetcher() override;

  // Supplied by the client for each hostlist request and returns a valid,
  // initialized Hostlist object on success.
  typedef base::Callback<void(const std::vector<HostInfo>& hostlist)>
    HostlistCallback;

  // Makes a service call to retrieve a hostlist. The
  // callback will be called once the HTTP request has completed.
  virtual void RetrieveHostlist(const std::string& access_token,
                                const std::string& target_url,
                                const HostlistCallback& callback);

 private:
  // Processes the response from the directory service.
  bool ProcessResponse(std::vector<HostInfo>* hostlist);

  // net::URLFetcherDelegate interface.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Holds the URLFetcher for the Host List request.
  std::unique_ptr<net::URLFetcher> request_;

  // Provides application-specific context for the network request.
  scoped_refptr<remoting::URLRequestContextGetter> request_context_getter_;

  // Caller-supplied callback used to return hostlist on success.
  HostlistCallback hostlist_callback_;

  DISALLOW_COPY_AND_ASSIGN(HostListFetcher);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_HOST_LIST_FETCHER_H_
