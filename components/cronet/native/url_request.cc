// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/url_request.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "components/cronet/native/engine.h"
#include "components/cronet/native/generated/cronet.idl_impl_struct.h"
#include "components/cronet/native/include/cronet_c.h"
#include "components/cronet/native/io_buffer_with_cronet_buffer.h"
#include "components/cronet/native/runnables.h"
#include "net/base/io_buffer.h"

namespace {

net::RequestPriority ConvertRequestPriority(
    Cronet_UrlRequestParams_REQUEST_PRIORITY priority) {
  switch (priority) {
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_IDLE:
      return net::IDLE;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOWEST:
      return net::LOWEST;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOW:
      return net::LOW;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_MEDIUM:
      return net::MEDIUM;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_HIGHEST:
      return net::HIGHEST;
  }
  return net::DEFAULT_PRIORITY;
}

std::unique_ptr<Cronet_UrlResponseInfo> CreateCronet_UrlResponseInfo(
    const std::vector<std::string>& url_chain,
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server,
    int64_t received_byte_count) {
  auto response_info = std::make_unique<Cronet_UrlResponseInfo>();
  response_info->url = url_chain.back();
  response_info->url_chain = url_chain;
  response_info->http_status_code = http_status_code;
  response_info->http_status_text = http_status_text;
  // |headers| could be nullptr.
  if (headers != nullptr) {
    size_t iter = 0;
    std::string header_name;
    std::string header_value;
    while (headers->EnumerateHeaderLines(&iter, &header_name, &header_value)) {
      Cronet_HttpHeader header;
      header.name = header_name;
      header.value = header_value;
      response_info->all_headers_list.push_back(std::move(header));
    }
  }
  response_info->was_cached = was_cached;
  response_info->negotiated_protocol = negotiated_protocol;
  response_info->proxy_server = proxy_server;
  response_info->received_byte_count = received_byte_count;
  return response_info;
}

Cronet_Error_ERROR_CODE NetErrorToCronetErrorCode(int net_error) {
  switch (net_error) {
    case net::ERR_NAME_NOT_RESOLVED:
      return Cronet_Error_ERROR_CODE_ERROR_HOSTNAME_NOT_RESOLVED;
    case net::ERR_INTERNET_DISCONNECTED:
      return Cronet_Error_ERROR_CODE_ERROR_INTERNET_DISCONNECTED;
    case net::ERR_NETWORK_CHANGED:
      return Cronet_Error_ERROR_CODE_ERROR_NETWORK_CHANGED;
    case net::ERR_TIMED_OUT:
      return Cronet_Error_ERROR_CODE_ERROR_TIMED_OUT;
    case net::ERR_CONNECTION_CLOSED:
      return Cronet_Error_ERROR_CODE_ERROR_CONNECTION_CLOSED;
    case net::ERR_CONNECTION_TIMED_OUT:
      return Cronet_Error_ERROR_CODE_ERROR_CONNECTION_TIMED_OUT;
    case net::ERR_CONNECTION_REFUSED:
      return Cronet_Error_ERROR_CODE_ERROR_CONNECTION_REFUSED;
    case net::ERR_CONNECTION_RESET:
      return Cronet_Error_ERROR_CODE_ERROR_CONNECTION_RESET;
    case net::ERR_ADDRESS_UNREACHABLE:
      return Cronet_Error_ERROR_CODE_ERROR_ADDRESS_UNREACHABLE;
    case net::ERR_QUIC_PROTOCOL_ERROR:
      return Cronet_Error_ERROR_CODE_ERROR_QUIC_PROTOCOL_FAILED;
    default:
      return Cronet_Error_ERROR_CODE_ERROR_OTHER;
  }
}

bool IsCronetErrorImmediatelyRetryable(Cronet_Error_ERROR_CODE error_code) {
  switch (error_code) {
    case Cronet_Error_ERROR_CODE_ERROR_HOSTNAME_NOT_RESOLVED:
    case Cronet_Error_ERROR_CODE_ERROR_INTERNET_DISCONNECTED:
    case Cronet_Error_ERROR_CODE_ERROR_CONNECTION_REFUSED:
    case Cronet_Error_ERROR_CODE_ERROR_ADDRESS_UNREACHABLE:
    case Cronet_Error_ERROR_CODE_ERROR_OTHER:
    default:
      return false;
    case Cronet_Error_ERROR_CODE_ERROR_NETWORK_CHANGED:
    case Cronet_Error_ERROR_CODE_ERROR_TIMED_OUT:
    case Cronet_Error_ERROR_CODE_ERROR_CONNECTION_CLOSED:
    case Cronet_Error_ERROR_CODE_ERROR_CONNECTION_TIMED_OUT:
    case Cronet_Error_ERROR_CODE_ERROR_CONNECTION_RESET:
      return true;
  }
}

std::unique_ptr<Cronet_Error> CreateCronet_Error(
    int net_error,
    int quic_error,
    const std::string& error_string) {
  auto error = std::make_unique<Cronet_Error>();
  error->error_code = NetErrorToCronetErrorCode(net_error);
  error->message = error_string;
  error->internal_error_code = net_error;
  error->quic_detailed_error_code = quic_error;
  error->immediately_retryable =
      IsCronetErrorImmediatelyRetryable(error->error_code);
  return error;
}

}  // namespace

namespace cronet {

// Callback is owned by CronetURLRequest. It is constructed on client thread,
// but invoked and deleted on the network thread.
class Cronet_UrlRequestImpl::Callback : public CronetURLRequest::Callback {
 public:
  Callback(const std::string& url,
           Cronet_UrlRequestImpl* url_request,
           Cronet_UrlRequestCallbackPtr callback,
           Cronet_ExecutorPtr executor);
  ~Callback() override = default;
  // CronetURLRequest::Callback implementations:
  void OnReceivedRedirect(const std::string& new_location,
                          int http_status_code,
                          const std::string& http_status_text,
                          const net::HttpResponseHeaders* headers,
                          bool was_cached,
                          const std::string& negotiated_protocol,
                          const std::string& proxy_server,
                          int64_t received_byte_count) override;
  void OnResponseStarted(int http_status_code,
                         const std::string& http_status_text,
                         const net::HttpResponseHeaders* headers,
                         bool was_cached,
                         const std::string& negotiated_protocol,
                         const std::string& proxy_server,
                         int64_t received_byte_count) override;
  void OnReadCompleted(scoped_refptr<net::IOBuffer> buffer,
                       int bytes_read,
                       int64_t received_byte_count) override;
  void OnSucceeded(int64_t received_byte_count) override;
  void OnError(int net_error,
               int quic_error,
               const std::string& error_string,
               int64_t received_byte_count) override;
  void OnCanceled() override;
  void OnDestroyed() override;
  void OnMetricsCollected(const base::Time& request_start_time,
                          const base::TimeTicks& request_start,
                          const base::TimeTicks& dns_start,
                          const base::TimeTicks& dns_end,
                          const base::TimeTicks& connect_start,
                          const base::TimeTicks& connect_end,
                          const base::TimeTicks& ssl_start,
                          const base::TimeTicks& ssl_end,
                          const base::TimeTicks& send_start,
                          const base::TimeTicks& send_end,
                          const base::TimeTicks& push_start,
                          const base::TimeTicks& push_end,
                          const base::TimeTicks& receive_headers_end,
                          const base::TimeTicks& request_end,
                          bool socket_reused,
                          int64_t sent_bytes_count,
                          int64_t received_bytes_count) override;

 private:
  void PostTaskToExecutor(base::OnceClosure task);

  // The UrlRequest which owns context that owns the callback.
  Cronet_UrlRequestImpl* url_request_ = nullptr;

  // Application callback interface, used, but not owned, by |this|.
  Cronet_UrlRequestCallbackPtr callback_ = nullptr;
  // Executor for application callback, used, but not owned, by |this|.
  Cronet_ExecutorPtr executor_ = nullptr;

  // URL chain contains the URL currently being requested, and
  // all URLs previously requested. New URLs are added before
  // Cronet_UrlRequestCallback::OnRedirectReceived is called.
  std::vector<std::string> url_chain_;

  // All methods except constructor are invoked on the network thread.
  THREAD_CHECKER(network_thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(Callback);
};

Cronet_UrlRequestImpl::Cronet_UrlRequestImpl() = default;

Cronet_UrlRequestImpl::~Cronet_UrlRequestImpl() {
  base::AutoLock lock(lock_);
  // Request may already be destroyed if it hasn't started or got canceled.
  if (request_)
    request_->Destroy(false);
}

Cronet_RESULT Cronet_UrlRequestImpl::InitWithParams(
    Cronet_EnginePtr engine,
    Cronet_String url,
    Cronet_UrlRequestParamsPtr params,
    Cronet_UrlRequestCallbackPtr callback,
    Cronet_ExecutorPtr executor) {
  CHECK(engine);
  engine_ = reinterpret_cast<Cronet_EngineImpl*>(engine);
  if (!url || std::string(url).empty())
    return engine_->CheckResult(Cronet_RESULT_NULL_POINTER_URL);
  if (!params)
    return engine_->CheckResult(Cronet_RESULT_NULL_POINTER_PARAMS);
  if (!callback)
    return engine_->CheckResult(Cronet_RESULT_NULL_POINTER_CALLBACK);
  if (!executor)
    return engine_->CheckResult(Cronet_RESULT_NULL_POINTER_EXECUTOR);

  VLOG(1) << "New Cronet_UrlRequest: " << url;

  // Tests call InitWithParams() repeatedly on the same Cronet_UrlRequestPtr,
  // rather than Destroy()ing and creating a new one, so ensure that any prior
  // |request_| is not leaked (see https://crbug.com/829077).
  {
    base::AutoLock lock(lock_);
    if (request_)
      request_->Destroy(false);
  }

  request_ = new CronetURLRequest(
      engine_->cronet_url_request_context(),
      std::make_unique<Callback>(url, this, callback, executor), GURL(url),
      ConvertRequestPriority(params->priority), params->disable_cache,
      true /* params->disableConnectionMigration */,
      false /* params->enableMetrics */,
      // TODO(pauljensen): Consider exposing TrafficStats API via C++ API.
      false /* traffic_stats_tag_set */, 0 /* traffic_stats_tag */,
      false /* traffic_stats_uid_set */, 0 /* traffic_stats_uid */);

  if (!params->http_method.empty() &&
      !request_->SetHttpMethod(params->http_method)) {
    return engine_->CheckResult(
        Cronet_RESULT_ILLEGAL_ARGUMENT_INVALID_HTTP_METHOD);
  }

  for (const auto& request_header : params->request_headers) {
    if (request_header.name.empty())
      return engine_->CheckResult(Cronet_RESULT_NULL_POINTER_HEADER_NAME);
    if (request_header.value.empty())
      return engine_->CheckResult(Cronet_RESULT_NULL_POINTER_HEADER_VALUE);
    if (!request_->AddRequestHeader(request_header.name,
                                    request_header.value)) {
      return engine_->CheckResult(
          Cronet_RESULT_ILLEGAL_ARGUMENT_INVALID_HTTP_HEADER);
    }
  }
  return engine_->CheckResult(Cronet_RESULT_SUCCESS);
}

Cronet_RESULT Cronet_UrlRequestImpl::Start() {
  base::AutoLock lock(lock_);
  if (started_) {
    return engine_->CheckResult(
        Cronet_RESULT_ILLEGAL_STATE_REQUEST_ALREADY_STARTED);
  }
  if (!request_) {
    return engine_->CheckResult(
        Cronet_RESULT_ILLEGAL_STATE_REQUEST_NOT_INITIALIZED);
  }
  request_->Start();
  started_ = true;
  return engine_->CheckResult(Cronet_RESULT_SUCCESS);
}

Cronet_RESULT Cronet_UrlRequestImpl::FollowRedirect() {
  base::AutoLock lock(lock_);
  if (!waiting_on_redirect_) {
    return engine_->CheckResult(
        Cronet_RESULT_ILLEGAL_STATE_UNEXPECTED_REDIRECT);
  }
  waiting_on_redirect_ = false;
  if (!IsDoneLocked())
    request_->FollowDeferredRedirect();
  return engine_->CheckResult(Cronet_RESULT_SUCCESS);
}

Cronet_RESULT Cronet_UrlRequestImpl::Read(Cronet_BufferPtr buffer) {
  base::AutoLock lock(lock_);
  if (!waiting_on_read_)
    return engine_->CheckResult(Cronet_RESULT_ILLEGAL_STATE_UNEXPECTED_READ);
  waiting_on_read_ = false;
  if (IsDoneLocked())
    return engine_->CheckResult(Cronet_RESULT_SUCCESS);
  // Create IOBuffer that will own |buffer| while it is used by |request_|.
  net::IOBuffer* io_buffer = new IOBufferWithCronet_Buffer(buffer);
  if (request_->ReadData(io_buffer, Cronet_Buffer_GetSize(buffer)))
    return engine_->CheckResult(Cronet_RESULT_SUCCESS);
  return engine_->CheckResult(Cronet_RESULT_ILLEGAL_STATE_READ_FAILED);
}

void Cronet_UrlRequestImpl::Cancel() {
  base::AutoLock lock(lock_);
  if (started_) {
    // If request has posted callbacks to client executor, then it is possible
    // that |request_| will be destroyed before callback is executed. The
    // callback runnable uses IsDone() to avoid calling client callback in this
    // case.
    DestroyRequestUnlessDoneLocked(
        Cronet_RequestFinishedInfo_FINISHED_REASON_CANCELED);
  }
}

bool Cronet_UrlRequestImpl::IsDone() {
  base::AutoLock lock(lock_);
  return IsDoneLocked();
}

bool Cronet_UrlRequestImpl::IsDoneLocked() {
  lock_.AssertAcquired();
  return started_ && request_ == nullptr;
}

bool Cronet_UrlRequestImpl::DestroyRequestUnlessDone(
    Cronet_RequestFinishedInfo_FINISHED_REASON finished_reason) {
  base::AutoLock lock(lock_);
  return DestroyRequestUnlessDoneLocked(finished_reason);
}

bool Cronet_UrlRequestImpl::DestroyRequestUnlessDoneLocked(
    Cronet_RequestFinishedInfo_FINISHED_REASON finished_reason) {
  lock_.AssertAcquired();
  if (request_ == nullptr)
    return true;
  DCHECK(error_ == nullptr ||
         finished_reason == Cronet_RequestFinishedInfo_FINISHED_REASON_FAILED);
  request_->Destroy(finished_reason ==
                    Cronet_RequestFinishedInfo_FINISHED_REASON_CANCELED);
  // Request can no longer be used as CronetURLRequest::Destroy() will
  // eventually delete |request_| from the network thread, so setting |request_|
  // to nullptr doesn't introduce a memory leak.
  request_ = nullptr;
  return false;
}

void Cronet_UrlRequestImpl::GetStatus(
    Cronet_UrlRequestStatusListenerPtr listener) {
  NOTIMPLEMENTED();
}

Cronet_UrlRequestImpl::Callback::Callback(const std::string& url,
                                          Cronet_UrlRequestImpl* url_request,
                                          Cronet_UrlRequestCallbackPtr callback,
                                          Cronet_ExecutorPtr executor)
    : url_request_(url_request),
      callback_(callback),
      executor_(executor),
      url_chain_({url}) {
  DETACH_FROM_THREAD(network_thread_checker_);
  DCHECK(url_request);
  DCHECK(callback);
  DCHECK(executor);
}

// CronetURLRequest::Callback implementations:
void Cronet_UrlRequestImpl::Callback::OnReceivedRedirect(
    const std::string& new_location,
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server,
    int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  base::AutoLock lock(url_request_->lock_);
  url_request_->waiting_on_redirect_ = true;
  url_request_->response_info_ = CreateCronet_UrlResponseInfo(
      url_chain_, http_status_code, http_status_text, headers, was_cached,
      negotiated_protocol, proxy_server, received_byte_count);
  // Have to do this after creating responseInfo.
  url_chain_.push_back(new_location);

  // Invoke Cronet_UrlRequestCallback_OnRedrectReceived using OnceClosure.
  PostTaskToExecutor(base::BindOnce(
      [](Cronet_UrlRequestCallback* callback,
         Cronet_UrlRequestImpl* url_request) {
        if (url_request->IsDone())
          return;
        Cronet_UrlRequestCallback_OnRedirectReceived(
            callback, url_request, url_request->response_info_.get(),
            url_request->response_info_->url_chain.front().c_str());
      },
      callback_, url_request_));
}

void Cronet_UrlRequestImpl::Callback::OnResponseStarted(
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server,
    int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  base::AutoLock lock(url_request_->lock_);
  url_request_->waiting_on_read_ = true;
  url_request_->response_info_ = CreateCronet_UrlResponseInfo(
      url_chain_, http_status_code, http_status_text, headers, was_cached,
      negotiated_protocol, proxy_server, received_byte_count);

  // Invoke Cronet_UrlRequestCallback_OnResponseStarted using OnceClosure.
  PostTaskToExecutor(base::BindOnce(
      [](Cronet_UrlRequestCallback* callback,
         Cronet_UrlRequestImpl* url_request) {
        if (url_request->IsDone())
          return;
        Cronet_UrlRequestCallback_OnResponseStarted(
            callback, url_request, url_request->response_info_.get());
      },
      callback_, url_request_));
}

void Cronet_UrlRequestImpl::Callback::OnReadCompleted(
    scoped_refptr<net::IOBuffer> buffer,
    int bytes_read,
    int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  IOBufferWithCronet_Buffer* io_buffer =
      reinterpret_cast<IOBufferWithCronet_Buffer*>(buffer.get());
  std::unique_ptr<Cronet_Buffer> cronet_buffer(io_buffer->Release());
  base::AutoLock lock(url_request_->lock_);
  url_request_->waiting_on_read_ = true;
  url_request_->response_info_->received_byte_count = received_byte_count;

  // Invoke Cronet_UrlRequestCallback_OnReadCompleted using OnceClosure.
  PostTaskToExecutor(base::BindOnce(
      [](Cronet_UrlRequestCallback* callback,
         Cronet_UrlRequestImpl* url_request,
         std::unique_ptr<Cronet_Buffer> cronet_buffer, int bytes_read) {
        if (url_request->IsDone())
          return;

        Cronet_UrlRequestCallback_OnReadCompleted(
            callback, url_request, url_request->response_info_.get(),
            cronet_buffer.release(), bytes_read);
      },
      callback_, url_request_, std::move(cronet_buffer), bytes_read));
}

void Cronet_UrlRequestImpl::Callback::OnSucceeded(int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  base::AutoLock lock(url_request_->lock_);
  url_request_->response_info_->received_byte_count = received_byte_count;

  // Invoke Cronet_UrlRequestCallback_OnSucceeded using OnceClosure.
  PostTaskToExecutor(base::BindOnce(
      [](Cronet_UrlRequestCallback* callback,
         Cronet_UrlRequestImpl* url_request) {
        if (url_request->DestroyRequestUnlessDone(
                Cronet_RequestFinishedInfo_FINISHED_REASON_SUCCEEDED)) {
          return;
        }
        Cronet_UrlRequestCallback_OnSucceeded(
            callback, url_request, url_request->response_info_.get());
      },
      callback_, url_request_));
}

void Cronet_UrlRequestImpl::Callback::OnError(int net_error,
                                              int quic_error,
                                              const std::string& error_string,
                                              int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  base::AutoLock lock(url_request_->lock_);
  if (url_request_->response_info_)
    url_request_->response_info_->received_byte_count = received_byte_count;

  url_request_->error_ =
      CreateCronet_Error(net_error, quic_error, error_string);

  // Invoke Cronet_UrlRequestCallback_OnFailed using OnceClosure.
  PostTaskToExecutor(base::BindOnce(
      [](Cronet_UrlRequestCallback* callback,
         Cronet_UrlRequestImpl* url_request) {
        if (url_request->DestroyRequestUnlessDone(
                Cronet_RequestFinishedInfo_FINISHED_REASON_FAILED)) {
          return;
        }
        Cronet_UrlRequestCallback_OnFailed(callback, url_request,
                                           url_request->response_info_.get(),
                                           url_request->error_.get());
      },
      callback_, url_request_));
}

void Cronet_UrlRequestImpl::Callback::OnCanceled() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  PostTaskToExecutor(base::BindOnce(
      [](Cronet_UrlRequestCallback* callback,
         Cronet_UrlRequestImpl* url_request) {
        Cronet_UrlRequestCallback_OnCanceled(callback, url_request,
                                             url_request->response_info_.get());
      },
      callback_, url_request_));
}

void Cronet_UrlRequestImpl::Callback::OnDestroyed() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  DCHECK(url_request_);
}

void Cronet_UrlRequestImpl::Callback::OnMetricsCollected(
    const base::Time& request_start_time,
    const base::TimeTicks& request_start,
    const base::TimeTicks& dns_start,
    const base::TimeTicks& dns_end,
    const base::TimeTicks& connect_start,
    const base::TimeTicks& connect_end,
    const base::TimeTicks& ssl_start,
    const base::TimeTicks& ssl_end,
    const base::TimeTicks& send_start,
    const base::TimeTicks& send_end,
    const base::TimeTicks& push_start,
    const base::TimeTicks& push_end,
    const base::TimeTicks& receive_headers_end,
    const base::TimeTicks& request_end,
    bool socket_reused,
    int64_t sent_bytes_count,
    int64_t received_bytes_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
}

void Cronet_UrlRequestImpl::Callback::PostTaskToExecutor(
    base::OnceClosure task) {
  Cronet_RunnablePtr runnable =
      new cronet::OnceClosureRunnable(std::move(task));
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

};  // namespace cronet

CRONET_EXPORT Cronet_UrlRequestPtr Cronet_UrlRequest_Create() {
  return new cronet::Cronet_UrlRequestImpl();
}
