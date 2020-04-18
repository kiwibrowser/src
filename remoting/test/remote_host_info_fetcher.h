// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_REMOTE_HOST_INFO_FETCHER_H_
#define REMOTING_TEST_REMOTE_HOST_INFO_FETCHER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "remoting/test/app_remoting_service_urls.h"
#include "remoting/test/remote_host_info.h"

namespace remoting {
class URLRequestContextGetter;
}

namespace remoting {
namespace test {

// Supplied by the client for each remote host info request and returns a valid,
// initialized RemoteHostInfo object on success.
typedef base::Callback<void(const RemoteHostInfo& remote_host_info)>
    RemoteHostInfoCallback;

// Calls the App Remoting service API to request connection info for a remote
// host.  Destroying the RemoteHostInfoFetcher while a request is outstanding
// will cancel the request. It is safe to delete the fetcher from within a
// completion callback.  Must be used from a thread running an IO message loop.
// The public method is virtual to allow for mocking and fakes.
class RemoteHostInfoFetcher : public net::URLFetcherDelegate {
 public:
  RemoteHostInfoFetcher();
  ~RemoteHostInfoFetcher() override;

  // Makes a service call to retrieve the details for a remote host.  The
  // callback will be called once the HTTP request has completed.
  virtual bool RetrieveRemoteHostInfo(const std::string& application_id,
                                      const std::string& access_token,
                                      ServiceEnvironment service_environment,
                                      const RemoteHostInfoCallback& callback);

 private:
  // net::URLFetcherDelegate interface.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Holds the URLFetcher for the RemoteHostInfo request.
  std::unique_ptr<net::URLFetcher> request_;

  // Provides application-specific context for the network request.
  scoped_refptr<remoting::URLRequestContextGetter> request_context_getter_;

  // Caller-supplied callback used to return remote host info on success.
  RemoteHostInfoCallback remote_host_info_callback_;

  DISALLOW_COPY_AND_ASSIGN(RemoteHostInfoFetcher);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_REMOTE_HOST_INFO_FETCHER_H_
