// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/sync_load_context.h"

#include <string>

#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "content/public/common/url_loader_throttle.h"
#include "content/renderer/loader/navigation_response_override_parameters.h"
#include "content/renderer/loader/sync_load_response.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response_info.h"

namespace content {

// An inner helper class to manage the SyncLoadContext's events and timeouts,
// so that we can stop or resumse all of them at once.
class SyncLoadContext::SignalHelper final {
 public:
  SignalHelper(SyncLoadContext* context,
               base::WaitableEvent* redirect_or_response_event,
               base::WaitableEvent* abort_event,
               double timeout)
      : context_(context),
        redirect_or_response_event_(redirect_or_response_event),
        abort_event_(abort_event) {
    Start(base::TimeDelta::FromSecondsD(timeout));
  }

  void SignalRedirectOrResponseComplete() {
    abort_watcher_.StopWatching();
    timeout_timer_.AbandonAndStop();
    redirect_or_response_event_->Signal();
  }

  bool RestartAfterRedirect() {
    if (abort_event_ && abort_event_->IsSignaled())
      return false;
    base::TimeDelta timeout_remainder =
        timeout_timer_.desired_run_time() - base::TimeTicks::Now();
    if (timeout_remainder <= base::TimeDelta())
      return false;
    Start(timeout_remainder);
    return true;
  }

 private:
  void Start(const base::TimeDelta& timeout) {
    DCHECK(!redirect_or_response_event_->IsSignaled());
    if (abort_event_) {
      abort_watcher_.StartWatching(
          abort_event_,
          base::BindOnce(&SyncLoadContext::OnAbort, base::Unretained(context_)),
          context_->task_runner_);
    }
    if (timeout > base::TimeDelta()) {
      timeout_timer_.Start(FROM_HERE, timeout, context_,
                           &SyncLoadContext::OnTimeout);
    }
  }

  SyncLoadContext* context_;
  base::WaitableEvent* redirect_or_response_event_;
  base::WaitableEvent* abort_event_;
  base::WaitableEventWatcher abort_watcher_;
  base::OneShotTimer timeout_timer_;
};

// static
void SyncLoadContext::StartAsyncWithWaitableEvent(
    std::unique_ptr<network::ResourceRequest> request,
    int routing_id,
    scoped_refptr<base::SingleThreadTaskRunner> loading_task_runner,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    std::unique_ptr<network::SharedURLLoaderFactoryInfo>
        url_loader_factory_info,
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
    SyncLoadResponse* response,
    base::WaitableEvent* redirect_or_response_event,
    base::WaitableEvent* abort_event,
    double timeout,
    blink::mojom::BlobRegistryPtrInfo download_to_blob_registry) {
  bool download_to_blob = download_to_blob_registry.is_valid();
  auto* context = new SyncLoadContext(
      request.get(), std::move(url_loader_factory_info), response,
      redirect_or_response_event, abort_event, timeout,
      std::move(download_to_blob_registry), loading_task_runner);
  context->request_id_ = context->resource_dispatcher_->StartAsync(
      std::move(request), routing_id, std::move(loading_task_runner),
      traffic_annotation, true /* is_sync */,
      download_to_blob /* pass_response_pipe_to_peer */,
      base::WrapUnique(context), context->url_loader_factory_,
      std::move(throttles), nullptr /* navigation_response_override_params */,
      nullptr /* continue_for_navigation */);
}

SyncLoadContext::SyncLoadContext(
    network::ResourceRequest* request,
    std::unique_ptr<network::SharedURLLoaderFactoryInfo> url_loader_factory,
    SyncLoadResponse* response,
    base::WaitableEvent* redirect_or_response_event,
    base::WaitableEvent* abort_event,
    double timeout,
    blink::mojom::BlobRegistryPtrInfo download_to_blob_registry,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : response_(response),
      download_to_blob_registry_(std::move(download_to_blob_registry)),
      task_runner_(std::move(task_runner)),
      signals_(std::make_unique<SignalHelper>(this,
                                              redirect_or_response_event,
                                              abort_event,
                                              timeout)),
      fetch_request_mode_(request->fetch_request_mode) {
  url_loader_factory_ =
      network::SharedURLLoaderFactory::Create(std::move(url_loader_factory));

  // Constructs a new ResourceDispatcher specifically for this request.
  resource_dispatcher_ = std::make_unique<ResourceDispatcher>();

  // Initialize the final URL with the original request URL. It will be
  // overwritten on redirects.
  response_->url = request->url;
}

SyncLoadContext::~SyncLoadContext() {}

void SyncLoadContext::OnUploadProgress(uint64_t position, uint64_t size) {}

bool SyncLoadContext::OnReceivedRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseInfo& info) {
  DCHECK(!Completed());
  // Synchronous loads in blink aren't associated with a ResourceClient, and
  // CORS checks are performed by ResourceClient subclasses, so there's
  // currently no way to perform CORS checks for redirects.
  // Err on the side of extreme caution and block any cross origin redirect
  // that might have CORS implications.
  if (fetch_request_mode_ != network::mojom::FetchRequestMode::kNoCORS &&
      redirect_info.new_url.GetOrigin() != response_->url.GetOrigin()) {
    LOG(ERROR) << "Cross origin redirect denied";
    response_->error_code = net::ERR_ABORTED;

    CompleteRequest(false /* remove_pending_request */);

    // Returning false here will cause the request to be cancelled and this
    // object deleted.
    return false;
  }

  response_->url = redirect_info.new_url;
  response_->info = info;
  response_->redirect_info = redirect_info;
  response_->context_for_redirect = this;
  resource_dispatcher_->SetDefersLoading(request_id_, true);
  signals_->SignalRedirectOrResponseComplete();
  return true;
}

void SyncLoadContext::FollowRedirect() {
  if (!signals_->RestartAfterRedirect()) {
    CancelRedirect();
    return;
  }

  response_->redirect_info = net::RedirectInfo();
  response_->context_for_redirect = nullptr;

  resource_dispatcher_->SetDefersLoading(request_id_, false);
}

void SyncLoadContext::CancelRedirect() {
  response_->redirect_info = net::RedirectInfo();
  response_->context_for_redirect = nullptr;
  response_->error_code = net::ERR_ABORTED;
  CompleteRequest(true);
}

void SyncLoadContext::OnReceivedResponse(
    const network::ResourceResponseInfo& info) {
  DCHECK(!Completed());
  response_->info = info;
}

void SyncLoadContext::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  DCHECK(download_to_blob_registry_);
  DCHECK(!blob_response_started_);

  blob_response_started_ = true;

  download_to_blob_registry_->RegisterFromStream(
      response_->info.mime_type, "",
      std::max<int64_t>(0, response_->info.content_length), std::move(body),
      nullptr,
      base::BindOnce(&SyncLoadContext::OnFinishCreatingBlob,
                     base::Unretained(this)));
}

void SyncLoadContext::OnDownloadedData(int len, int encoded_data_length) {
  downloaded_file_length_ =
      (downloaded_file_length_ ? *downloaded_file_length_ : 0) + len;
}

void SyncLoadContext::OnReceivedData(std::unique_ptr<ReceivedData> data) {
  DCHECK(!Completed());
  response_->data.append(data->payload(), data->length());
}

void SyncLoadContext::OnTransferSizeUpdated(int transfer_size_diff) {}

void SyncLoadContext::OnCompletedRequest(
    const network::URLLoaderCompletionStatus& status) {
  DCHECK(!Completed());
  response_->error_code = status.error_code;
  response_->extended_error_code = status.extended_error_code;
  if (status.cors_error_status)
    response_->cors_error = status.cors_error_status->cors_error;
  response_->info.encoded_data_length = status.encoded_data_length;
  response_->info.encoded_body_length = status.encoded_body_length;
  response_->downloaded_file_length = downloaded_file_length_;
  // Need to pass |downloaded_tmp_file| to the caller thread. Otherwise the blob
  // creation in ResourceResponse::SetDownloadedFilePath() fails.
  response_->downloaded_tmp_file =
      resource_dispatcher_->TakeDownloadedTempFile(request_id_);
  DCHECK_EQ(!response_->downloaded_file_length,
            !response_->downloaded_tmp_file);
  if (blob_response_started_ && !blob_finished_) {
    request_completed_ = true;
    return;
  }
  CompleteRequest(true /* remove_pending_request */);
}

void SyncLoadContext::OnFinishCreatingBlob(
    blink::mojom::SerializedBlobPtr blob) {
  DCHECK(!Completed());
  blob_finished_ = true;
  response_->downloaded_blob = std::move(blob);
  if (request_completed_)
    CompleteRequest(true /* remove_pending_request */);
}

void SyncLoadContext::OnAbort(base::WaitableEvent* event) {
  DCHECK(!Completed());
  response_->error_code = net::ERR_ABORTED;
  CompleteRequest(true /* remove_pending_request */);
}

void SyncLoadContext::OnTimeout() {
  // OnTimeout() must not be called after CompleteRequest() was called, because
  // the OneShotTimer must have been stopped.
  DCHECK(!Completed());
  response_->error_code = net::ERR_TIMED_OUT;
  CompleteRequest(true /* remove_pending_request */);
}

void SyncLoadContext::CompleteRequest(bool remove_pending_request) {
  signals_->SignalRedirectOrResponseComplete();
  signals_ = nullptr;
  response_ = nullptr;

  if (remove_pending_request) {
    // This will indirectly cause this object to be deleted.
    resource_dispatcher_->RemovePendingRequest(request_id_, task_runner_);
  }
}

bool SyncLoadContext::Completed() const {
  DCHECK_EQ(!signals_, !response_);
  return !response_;
}

}  // namespace content
