// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/net/connectivity_checker_impl.h"

#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chromecast/base/metrics/cast_metrics_helper.h"
#include "chromecast/chromecast_buildflags.h"
#include "chromecast/net/net_switches.h"
#include "net/base/request_priority.h"
#include "net/http/http_network_session.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_status_code.h"
#include "net/http/http_transaction_factory.h"
#include "net/socket/ssl_client_socket.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"

namespace chromecast {

namespace {

// How often connectivity checks are performed in seconds while not connected.
const unsigned int kConnectivityPeriodSeconds = 1;

// How often connectivity checks are performed in seconds while connected.
const unsigned int kConnectivitySuccessPeriodSeconds = 60;

// Number of consecutive connectivity check errors before status is changed
// to offline.
const unsigned int kNumErrorsToNotifyOffline = 3;

// Request timeout value in seconds.
const unsigned int kRequestTimeoutInSeconds = 3;

// Default url for connectivity checking.
const char kDefaultConnectivityCheckUrl[] =
    "https://connectivitycheck.gstatic.com/generate_204";

// Delay notification of network change events to smooth out rapid flipping.
// Histogram "Cast.Network.Down.Duration.In.Seconds" shows 40% of network
// downtime is less than 3 seconds.
const char kNetworkChangedDelayInSeconds = 3;

const char kMetricNameNetworkConnectivityCheckingErrorType[] =
    "Network.ConnectivityChecking.ErrorType";

}  // namespace

ConnectivityCheckerImpl::ConnectivityCheckerImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    net::URLRequestContextGetter* url_request_context_getter)
    : ConnectivityChecker(),
      task_runner_(task_runner),
      connected_(false),
      connection_type_(net::NetworkChangeNotifier::CONNECTION_NONE),
      check_errors_(0),
      network_changed_pending_(false) {
  DCHECK(task_runner_.get());

  task_runner->PostTask(
      FROM_HERE, base::BindOnce(&ConnectivityCheckerImpl::Initialize, this,
                                base::RetainedRef(url_request_context_getter)));
}

void ConnectivityCheckerImpl::Initialize(
    net::URLRequestContextGetter* url_request_context_getter) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::CommandLine::StringType check_url_str =
      command_line->GetSwitchValueNative(switches::kConnectivityCheckUrl);
  connectivity_check_url_.reset(new GURL(
      check_url_str.empty() ? kDefaultConnectivityCheckUrl : check_url_str));

  url_request_context_ = url_request_context_getter->GetURLRequestContext();

  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&ConnectivityCheckerImpl::Check, this));
}

ConnectivityCheckerImpl::~ConnectivityCheckerImpl() {
  DCHECK(task_runner_.get());
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
  task_runner_->DeleteSoon(FROM_HERE, url_request_.release());
}

bool ConnectivityCheckerImpl::Connected() const {
  base::AutoLock auto_lock(connected_lock_);
  return connected_;
}

void ConnectivityCheckerImpl::SetConnected(bool connected) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (connected_ == connected)
    return;
  {
    base::AutoLock auto_lock(connected_lock_);
    connected_ = connected;
  }
  Notify(connected);
  LOG(INFO) << "Global connection is: " << (connected ? "Up" : "Down");
}

void ConnectivityCheckerImpl::Check() {
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ConnectivityCheckerImpl::CheckInternal, this));
}

void ConnectivityCheckerImpl::CheckInternal() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(url_request_context_);

  // Don't check connectivity if network is offline, because Internet could be
  // accessible via netifs ignored.
  if (net::NetworkChangeNotifier::IsOffline())
    return;

  // If url_request_ is non-null, there is already a check going on. Don't
  // start another.
  if (url_request_.get())
    return;

  VLOG(1) << "Connectivity check: url=" << *connectivity_check_url_;
  url_request_ = url_request_context_->CreateRequest(
      *connectivity_check_url_, net::MAXIMUM_PRIORITY, this);
  url_request_->set_method("HEAD");
  url_request_->Start();

  timeout_.Reset(base::Bind(&ConnectivityCheckerImpl::OnUrlRequestTimeout,
                            this));
  // Exponential backoff for timeout in 3, 6 and 12 sec.
  const int timeout = kRequestTimeoutInSeconds
                      << (check_errors_ > 2 ? 2 : check_errors_);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, timeout_.callback(), base::TimeDelta::FromSeconds(timeout));
}

void ConnectivityCheckerImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  VLOG(2) << "OnNetworkChanged " << type;
  connection_type_ = type;

  if (network_changed_pending_)
    return;
  network_changed_pending_ = true;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ConnectivityCheckerImpl::OnNetworkChangedInternal, this),
      base::TimeDelta::FromSeconds(kNetworkChangedDelayInSeconds));
}

void ConnectivityCheckerImpl::OnNetworkChangedInternal() {
  network_changed_pending_ = false;
  Cancel();

  if (connection_type_ == net::NetworkChangeNotifier::CONNECTION_NONE) {
    SetConnected(false);
    return;
  }

  Check();
}

void ConnectivityCheckerImpl::OnResponseStarted(net::URLRequest* request,
                                                int net_error) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  int http_response_code =
      (net_error == net::OK &&
       request->response_info().headers.get() != nullptr)
          ? request->response_info().headers->response_code()
          : net::HTTP_BAD_REQUEST;

  // Clears resources.
  // URLRequest::Cancel() is called in destructor.
  url_request_.reset(nullptr);

  if (http_response_code < 400) {
    VLOG(1) << "Connectivity check succeeded";
    check_errors_ = 0;
    SetConnected(true);
    // Some products don't have an idle screen that makes periodic network
    // requests. Schedule another check to ensure connectivity hasn't dropped.
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&ConnectivityCheckerImpl::CheckInternal, this),
        base::TimeDelta::FromSeconds(kConnectivitySuccessPeriodSeconds));
    timeout_.Cancel();
    return;
  }
  VLOG(1) << "Connectivity check failed: " << http_response_code;
  OnUrlRequestError(ErrorType::BAD_HTTP_STATUS);
  timeout_.Cancel();
}

void ConnectivityCheckerImpl::OnReadCompleted(net::URLRequest* request,
                                              int bytes_read) {
  NOTREACHED();
}

void ConnectivityCheckerImpl::OnSSLCertificateError(
    net::URLRequest* request,
    const net::SSLInfo& ssl_info,
    bool fatal) {
  if (url_request_context_->http_transaction_factory()
          ->GetSession()
          ->params()
          .ignore_certificate_errors) {
    LOG(WARNING) << "OnSSLCertificateError: ignore cert_status="
                 << ssl_info.cert_status;
    request->ContinueDespiteLastError();
    return;
  }
  DCHECK(task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << "OnSSLCertificateError: cert_status=" << ssl_info.cert_status;
  net::SSLClientSocket::ClearSessionCache();
  OnUrlRequestError(ErrorType::SSL_CERTIFICATE_ERROR);
  timeout_.Cancel();
}

void ConnectivityCheckerImpl::OnUrlRequestError(ErrorType type) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  ++check_errors_;
  if (check_errors_ > kNumErrorsToNotifyOffline) {
    // Only record event on the connectivity transition.
    if (connected_) {
      metrics::CastMetricsHelper::GetInstance()->RecordEventWithValue(
          kMetricNameNetworkConnectivityCheckingErrorType,
          static_cast<int>(type));
    }
    check_errors_ = kNumErrorsToNotifyOffline;
    SetConnected(false);
  }
  url_request_.reset(nullptr);
  // Check again.
  task_runner_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ConnectivityCheckerImpl::Check, this),
      base::TimeDelta::FromSeconds(kConnectivityPeriodSeconds));
}

void ConnectivityCheckerImpl::OnUrlRequestTimeout() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << "time out";
  OnUrlRequestError(ErrorType::REQUEST_TIMEOUT);
}

void ConnectivityCheckerImpl::Cancel() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (!url_request_.get())
    return;
  VLOG(2) << "Cancel connectivity check in progress";
  url_request_.reset(nullptr);  // URLRequest::Cancel() is called in destructor.
  timeout_.Cancel();
}

}  // namespace chromecast
