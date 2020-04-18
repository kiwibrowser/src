// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_FACADE_HOST_LIST_FETCHER_H_
#define REMOTING_IOS_FACADE_HOST_LIST_FETCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "remoting/ios/facade/host_info.h"

namespace remoting {

// Requests a host list from the directory service for an access token.
// Destroying the RemoteHostInfoFetcher while a request is outstanding
// will cancel the request. It is safe to delete the fetcher from within a
// completion callback.  Must be used from a thread running a message loop.
// The public method is virtual to allow for mocking and fakes.
class HostListFetcher : public net::URLFetcherDelegate {
 public:
  // Imposible http response code. Used to signal that the request has been
  // cancelled.
  // TODO(yuweih): Add all response code values here and make the callback
  // return this enum instead of int.
  enum ResponseCode {
    RESPONSE_CODE_CANCELLED = -257,
  };

  HostListFetcher(const scoped_refptr<net::URLRequestContextGetter>&
                      url_request_context_getter);
  ~HostListFetcher() override;

  // Supplied by the client for each hostlist request and returns a valid,
  // initialized Hostlist object on success.
  typedef base::OnceCallback<
      void(int response_code, const std::vector<remoting::HostInfo>& hostlist)>
      HostlistCallback;

  // Makes a service call to retrieve a hostlist. The
  // callback will be called once the HTTP request has completed.
  virtual void RetrieveHostlist(const std::string& access_token,
                                HostlistCallback callback);

  // Cancels the current fetch request and runs the host list callback with
  // RESPONSE_CODE_CANCELLED. Nothing will happen if there is no ongoing fetch
  // request.
  virtual void CancelFetch();

 private:
  // Processes the response from the directory service.
  bool ProcessResponse(std::vector<remoting::HostInfo>* hostlist);

  // net::URLFetcherDelegate interface.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // Holds the URLFetcher for the Host List request.
  std::unique_ptr<net::URLFetcher> request_;

  // Caller-supplied callback used to return hostlist on success.
  HostlistCallback hostlist_callback_;

  DISALLOW_COPY_AND_ASSIGN(HostListFetcher);
};

}  // namespace remoting

#endif  // REMOTING_IOS_FACADE_HOST_LIST_FETCHER_H_
