// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_NET_CONNECTIVITY_CHECKER_IMPL_H_
#define CHROMECAST_NET_CONNECTIVITY_CHECKER_IMPL_H_

#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "chromecast/net/connectivity_checker.h"
#include "net/base/network_change_notifier.h"
#include "net/url_request/url_request.h"

class GURL;

namespace base {
class SingleThreadTaskRunner;
}

namespace net {
class SSLInfo;
class URLRequest;
class URLRequestContext;
class URLRequestContextGetter;
}

namespace chromecast {

// Simple class to check network connectivity by sending a HEAD http request
// to given url.
class ConnectivityCheckerImpl
    : public ConnectivityChecker,
      public net::URLRequest::Delegate,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // Connectivity checking and initialization will run on task_runner.
  explicit ConnectivityCheckerImpl(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      net::URLRequestContextGetter* url_request_context_getter);

  // ConnectivityChecker implementation:
  bool Connected() const override;
  void Check() override;

 protected:
  ~ConnectivityCheckerImpl() override;

 private:
  // UrlRequest::Delegate implementation:
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;
  void OnSSLCertificateError(net::URLRequest* request,
                             const net::SSLInfo& ssl_info,
                             bool fatal) override;

  // Initializes ConnectivityChecker
  void Initialize(net::URLRequestContextGetter* url_request_context_getter);

  // net::NetworkChangeNotifier::NetworkChangeObserver implementation:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  void OnNetworkChangedInternal();

  // Cancels current connectivity checking in progress.
  void Cancel();

  // Sets connectivity and alerts observers if it has changed
  void SetConnected(bool connected);

  enum class ErrorType {
    BAD_HTTP_STATUS = 1,
    SSL_CERTIFICATE_ERROR = 2,
    REQUEST_TIMEOUT = 3,
  };

  // Called when URL request failed.
  void OnUrlRequestError(ErrorType type);

  // Called when URL request timed out.
  void OnUrlRequestTimeout();

  void CheckInternal();

  std::unique_ptr<GURL> connectivity_check_url_;
  net::URLRequestContext* url_request_context_;
  std::unique_ptr<net::URLRequest> url_request_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // connected_lock_ protects access to connected_ which is shared across
  // threads.
  mutable base::Lock connected_lock_;
  bool connected_;

  net::NetworkChangeNotifier::ConnectionType connection_type_;
  // Number of connectivity check errors.
  unsigned int check_errors_;
  bool network_changed_pending_;
  // Timeout handler for connectivity checks.
  // Note: Cancelling this timeout can cause the destructor for this class to be
  // called.
  base::CancelableCallback<void()> timeout_;

  DISALLOW_COPY_AND_ASSIGN(ConnectivityCheckerImpl);
};

}  // namespace chromecast

#endif  // CHROMECAST_NET_CONNECTIVITY_CHECKER_IMPL_H_
