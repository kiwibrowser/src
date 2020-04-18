// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/net/http_bridge.h"

#include <stddef.h>

#include <vector>

#include "base/bit_cast.h"
#include "base/location.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/sync/base/cancelation_signal.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_status.h"
#include "third_party/zlib/google/compression_utils.h"

namespace syncer {

namespace {

// It's possible for an http request to be silently stalled. We set a time
// limit for all http requests, beyond which the request is cancelled and
// treated as a transient failure.
const int kMaxHttpRequestTimeSeconds = 60 * 5;  // 5 minutes.

// Helper method for logging timeouts via UMA.
void LogTimeout(bool timed_out) {
  UMA_HISTOGRAM_BOOLEAN("Sync.URLFetchTimedOut", timed_out);
}

void RecordSyncRequestContentLengthHistograms(int64_t compressed_content_length,
                                              int64_t original_content_length) {
  UMA_HISTOGRAM_COUNTS("Sync.RequestContentLength.Compressed",
                       compressed_content_length);
  UMA_HISTOGRAM_COUNTS("Sync.RequestContentLength.Original",
                       original_content_length);
}

void RecordSyncResponseContentLengthHistograms(
    int64_t compressed_content_length,
    int64_t original_content_length) {
  UMA_HISTOGRAM_COUNTS("Sync.ResponseContentLength.Compressed",
                       compressed_content_length);
  UMA_HISTOGRAM_COUNTS("Sync.ResponseContentLength.Original",
                       original_content_length);
}

}  // namespace

HttpBridgeFactory::HttpBridgeFactory(
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    const NetworkTimeUpdateCallback& network_time_update_callback,
    CancelationSignal* cancelation_signal)
    : request_context_getter_(request_context_getter),
      network_time_update_callback_(network_time_update_callback),
      cancelation_signal_(cancelation_signal) {
  // This registration is happening on the Sync thread, while signalling occurs
  // on the UI thread. We must handle the possibility signalling has already
  // occurred.
  if (!cancelation_signal_->TryRegisterHandler(this)) {
    OnSignalReceived();
  }
}

HttpBridgeFactory::~HttpBridgeFactory() {
  cancelation_signal_->UnregisterHandler(this);
}

void HttpBridgeFactory::Init(
    const std::string& user_agent,
    const BindToTrackerCallback& bind_to_tracker_callback) {
  user_agent_ = user_agent;
  bind_to_tracker_callback_ = bind_to_tracker_callback;
}

HttpPostProviderInterface* HttpBridgeFactory::Create() {
  base::AutoLock lock(request_context_getter_lock_);

  // If we've been asked to shut down (something which may happen asynchronously
  // and at pretty much any time), then we won't have a request_context_getter_.
  // Some external mechanism must ensure that this function is not called after
  // we've been asked to shut down.
  DCHECK(request_context_getter_);

  scoped_refptr<HttpBridge> http =
      new HttpBridge(user_agent_, request_context_getter_,
                     network_time_update_callback_, bind_to_tracker_callback_);
  http->AddRef();
  return http.get();
}

void HttpBridgeFactory::Destroy(HttpPostProviderInterface* http) {
  static_cast<HttpBridge*>(http)->Release();
}

void HttpBridgeFactory::OnSignalReceived() {
  base::AutoLock lock(request_context_getter_lock_);
  // Release |request_context_getter_| as soon as possible so that it
  // is destroyed in the right order on its network task runner.
  request_context_getter_ = nullptr;
}

HttpBridge::URLFetchState::URLFetchState()
    : url_poster(nullptr),
      aborted(false),
      request_completed(false),
      request_succeeded(false),
      http_response_code(-1),
      error_code(-1) {}
HttpBridge::URLFetchState::~URLFetchState() {}

HttpBridge::HttpBridge(
    const std::string& user_agent,
    const scoped_refptr<net::URLRequestContextGetter>& context_getter,
    const NetworkTimeUpdateCallback& network_time_update_callback,
    const BindToTrackerCallback& bind_to_tracker_callback)
    : user_agent_(user_agent),
      http_post_completed_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED),
      request_context_getter_(context_getter),
      network_task_runner_(request_context_getter_->GetNetworkTaskRunner()),
      network_time_update_callback_(network_time_update_callback),
      bind_to_tracker_callback_(bind_to_tracker_callback) {}

HttpBridge::~HttpBridge() {}

void HttpBridge::SetExtraRequestHeaders(const char* headers) {
  DCHECK(extra_headers_.empty())
      << "HttpBridge::SetExtraRequestHeaders called twice.";
  extra_headers_.assign(headers);
}

void HttpBridge::SetURL(const char* url, int port) {
#if DCHECK_IS_ON()
  DCHECK(thread_checker_.CalledOnValidThread());
  {
    base::AutoLock lock(fetch_state_lock_);
    DCHECK(!fetch_state_.request_completed);
  }
  DCHECK(url_for_request_.is_empty())
      << "HttpBridge::SetURL called more than once?!";
#endif
  GURL temp(url);
  GURL::Replacements replacements;
  std::string port_str = base::IntToString(port);
  replacements.SetPort(port_str.c_str(), url::Component(0, port_str.length()));
  url_for_request_ = temp.ReplaceComponents(replacements);
}

void HttpBridge::SetPostPayload(const char* content_type,
                                int content_length,
                                const char* content) {
#if DCHECK_IS_ON()
  DCHECK(thread_checker_.CalledOnValidThread());
  {
    base::AutoLock lock(fetch_state_lock_);
    DCHECK(!fetch_state_.request_completed);
  }
  DCHECK(content_type_.empty()) << "Bridge payload already set.";
  DCHECK_GE(content_length, 0) << "Content length < 0";
#endif
  content_type_ = content_type;
  if (!content || (content_length == 0)) {
    DCHECK_EQ(content_length, 0);
    request_content_ = " ";  // TODO(timsteele): URLFetcher requires non-empty
                             // content for POSTs whereas CURL does not, for now
                             // we hack this to support the sync backend.
  } else {
    request_content_.assign(content, content_length);
  }
}

bool HttpBridge::MakeSynchronousPost(int* error_code, int* response_code) {
#if DCHECK_IS_ON()
  DCHECK(thread_checker_.CalledOnValidThread());
  {
    base::AutoLock lock(fetch_state_lock_);
    DCHECK(!fetch_state_.request_completed);
  }
  DCHECK(url_for_request_.is_valid()) << "Invalid URL for request";
  DCHECK(!content_type_.empty()) << "Payload not set";
#endif

  if (!network_task_runner_->PostTask(
          FROM_HERE, base::Bind(&HttpBridge::CallMakeAsynchronousPost, this))) {
    // This usually happens when we're in a unit test.
    LOG(WARNING) << "Could not post CallMakeAsynchronousPost task";
    return false;
  }

  // Block until network request completes or is aborted. See
  // OnURLFetchComplete and Abort.
  http_post_completed_.Wait();

  base::AutoLock lock(fetch_state_lock_);
  DCHECK(fetch_state_.request_completed || fetch_state_.aborted);
  *error_code = fetch_state_.error_code;
  *response_code = fetch_state_.http_response_code;
  return fetch_state_.request_succeeded;
}

void HttpBridge::MakeAsynchronousPost() {
  DCHECK(network_task_runner_->BelongsToCurrentThread());

  base::AutoLock lock(fetch_state_lock_);
  DCHECK(!fetch_state_.request_completed);
  if (fetch_state_.aborted)
    return;

  // Start the timer on the network thread (the same thread progress is made
  // on, and on which the url fetcher lives).
  DCHECK(!fetch_state_.http_request_timeout_timer);
  fetch_state_.http_request_timeout_timer =
      std::make_unique<base::Timer>(false, false);
  fetch_state_.http_request_timeout_timer->Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kMaxHttpRequestTimeSeconds),
      base::Bind(&HttpBridge::OnURLFetchTimedOut, this));

  DCHECK(request_context_getter_);
  fetch_state_.start_time = base::Time::Now();
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("sync_http_bridge", R"(
        semantics {
          sender: "Chrome Sync"
          description:
            "Chrome Sync synchronizes profile data between Chromium clients "
            "and Google for a given user account."
          trigger:
            "User makes a change to syncable profile data after enabling sync "
            "on the device."
          data:
            "The device and user identifiers, along with any profile data that "
            "is changing."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can disable Chrome Sync by going into the profile settings "
            "and choosing to Sign Out."
          chrome_policy {
            SyncDisabled {
              policy_options {mode: MANDATORY}
              SyncDisabled: true
            }
          }
        })");
  fetch_state_.url_poster =
      net::URLFetcher::Create(url_for_request_, net::URLFetcher::POST, this,
                              traffic_annotation)
          .release();
  if (!bind_to_tracker_callback_.is_null())
    bind_to_tracker_callback_.Run(fetch_state_.url_poster);
  fetch_state_.url_poster->SetRequestContext(request_context_getter_.get());
  fetch_state_.url_poster->SetExtraRequestHeaders(extra_headers_);

  std::string request_to_send;
  compression::GzipCompress(request_content_, &request_to_send);
  fetch_state_.url_poster->AddExtraRequestHeader("Content-Encoding: gzip");
  fetch_state_.url_poster->SetUploadData(content_type_, request_to_send);
  RecordSyncRequestContentLengthHistograms(request_to_send.size(),
                                           request_content_.size());

  fetch_state_.url_poster->AddExtraRequestHeader(base::StringPrintf(
      "%s: %s", net::HttpRequestHeaders::kUserAgent, user_agent_.c_str()));
  fetch_state_.url_poster->SetLoadFlags(
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SAVE_COOKIES | net::LOAD_DO_NOT_SEND_COOKIES);

  fetch_state_.url_poster->Start();
}

int HttpBridge::GetResponseContentLength() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  base::AutoLock lock(fetch_state_lock_);
  DCHECK(fetch_state_.request_completed);
  return fetch_state_.response_content.size();
}

const char* HttpBridge::GetResponseContent() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  base::AutoLock lock(fetch_state_lock_);
  DCHECK(fetch_state_.request_completed);
  return fetch_state_.response_content.data();
}

const std::string HttpBridge::GetResponseHeaderValue(
    const std::string& name) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  base::AutoLock lock(fetch_state_lock_);
  DCHECK(fetch_state_.request_completed);

  std::string value;
  fetch_state_.response_headers->EnumerateHeader(nullptr, name, &value);
  return value;
}

void HttpBridge::Abort() {
  base::AutoLock lock(fetch_state_lock_);

  // Release |request_context_getter_| as soon as possible so that it is
  // destroyed in the right order on its network task runner.
  request_context_getter_ = nullptr;

  DCHECK(!fetch_state_.aborted);
  if (fetch_state_.aborted || fetch_state_.request_completed)
    return;

  fetch_state_.aborted = true;
  if (!network_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&HttpBridge::DestroyURLFetcherOnIOThread, this,
                     fetch_state_.url_poster,
                     fetch_state_.http_request_timeout_timer.release()))) {
    // Madness ensues.
    NOTREACHED() << "Could not post task to delete URLFetcher";
  }

  fetch_state_.url_poster = nullptr;
  fetch_state_.error_code = net::ERR_ABORTED;
  http_post_completed_.Signal();
}

void HttpBridge::DestroyURLFetcherOnIOThread(net::URLFetcher* fetcher,
                                             base::Timer* fetch_timer) {
  DCHECK(network_task_runner_->BelongsToCurrentThread());

    delete fetch_timer;
  delete fetcher;
}

void HttpBridge::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(network_task_runner_->BelongsToCurrentThread());

  base::AutoLock lock(fetch_state_lock_);

  // Stop the request timer now that the request completed.
  if (fetch_state_.http_request_timeout_timer)
    fetch_state_.http_request_timeout_timer.reset();

  if (fetch_state_.aborted)
    return;

  fetch_state_.end_time = base::Time::Now();
  fetch_state_.request_completed = true;
  fetch_state_.request_succeeded =
      (net::URLRequestStatus::SUCCESS == source->GetStatus().status());
  fetch_state_.http_response_code = source->GetResponseCode();
  fetch_state_.error_code = source->GetStatus().error();

  if (fetch_state_.request_succeeded)
    LogTimeout(false);
  base::UmaHistogramSparse("Sync.URLFetchResponse",
                           source->GetStatus().is_success()
                               ? source->GetResponseCode()
                               : source->GetStatus().ToNetError());
  UMA_HISTOGRAM_LONG_TIMES("Sync.URLFetchTime",
                           fetch_state_.end_time - fetch_state_.start_time);

  // Use a real (non-debug) log to facilitate troubleshooting in the wild.
  VLOG(2) << "HttpBridge::OnURLFetchComplete for: "
          << fetch_state_.url_poster->GetURL().spec();
  VLOG(1) << "HttpBridge received response code: "
          << fetch_state_.http_response_code;

  source->GetResponseAsString(&fetch_state_.response_content);
  fetch_state_.response_headers = source->GetResponseHeaders();
  UpdateNetworkTime();

  int64_t original_content_length = fetch_state_.response_content.size();
  int64_t compressed_content_length =
      source->GetReceivedResponseContentLength();
  RecordSyncResponseContentLengthHistograms(compressed_content_length,
                                            original_content_length);

  // End of the line for url_poster_. It lives only on the IO loop.
  // We defer deletion because we're inside a callback from a component of the
  // URLFetcher, so it seems most natural / "polite" to let the stack unwind.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                  fetch_state_.url_poster);
  fetch_state_.url_poster = nullptr;

  // Wake the blocked syncer thread in MakeSynchronousPost.
  // WARNING: DONT DO ANYTHING AFTER THIS CALL! |this| may be deleted!
  http_post_completed_.Signal();
}

void HttpBridge::OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                            int64_t current,
                                            int64_t total,
                                            int64_t current_network_bytes) {
  DCHECK(network_task_runner_->BelongsToCurrentThread());
  // Reset the delay when forward progress is made.
  base::AutoLock lock(fetch_state_lock_);
  if (fetch_state_.http_request_timeout_timer)
    fetch_state_.http_request_timeout_timer->Reset();
}

void HttpBridge::OnURLFetchUploadProgress(const net::URLFetcher* source,
                                          int64_t current,
                                          int64_t total) {
  DCHECK(network_task_runner_->BelongsToCurrentThread());
  // Reset the delay when forward progress is made.
  base::AutoLock lock(fetch_state_lock_);
  if (fetch_state_.http_request_timeout_timer)
    fetch_state_.http_request_timeout_timer->Reset();
}

void HttpBridge::OnURLFetchTimedOut() {
  DCHECK(network_task_runner_->BelongsToCurrentThread());

  base::AutoLock lock(fetch_state_lock_);
  if (!fetch_state_.url_poster)
    return;

  LogTimeout(true);
  DVLOG(1) << "Sync url fetch timed out. Canceling.";

  fetch_state_.end_time = base::Time::Now();
  fetch_state_.request_completed = true;
  fetch_state_.request_succeeded = false;
  fetch_state_.http_response_code = -1;
  fetch_state_.error_code = net::ERR_TIMED_OUT;

  // This method is called by the timer, not the url fetcher implementation,
  // so it's safe to delete the fetcher here.
  delete fetch_state_.url_poster;
  fetch_state_.url_poster = nullptr;

  // Timer is smart enough to handle being deleted as part of the invoked task.
  fetch_state_.http_request_timeout_timer.reset();

  // Wake the blocked syncer thread in MakeSynchronousPost.
  // WARNING: DONT DO ANYTHING AFTER THIS CALL! |this| may be deleted!
  http_post_completed_.Signal();
}

net::URLRequestContextGetter* HttpBridge::GetRequestContextGetterForTest()
    const {
  base::AutoLock lock(fetch_state_lock_);
  return request_context_getter_.get();
}

void HttpBridge::UpdateNetworkTime() {
  std::string sane_time_str;
  if (!fetch_state_.request_succeeded || fetch_state_.start_time.is_null() ||
      fetch_state_.end_time < fetch_state_.start_time ||
      !fetch_state_.response_headers ||
      !fetch_state_.response_headers->EnumerateHeader(
          nullptr, "Sane-Time-Millis", &sane_time_str)) {
    return;
  }

  int64_t sane_time_ms = 0;
  if (base::StringToInt64(sane_time_str, &sane_time_ms)) {
    network_time_update_callback_.Run(
        base::Time::FromJsTime(sane_time_ms),
        base::TimeDelta::FromMilliseconds(1),
        fetch_state_.end_time - fetch_state_.start_time);
  }
}

}  // namespace syncer
