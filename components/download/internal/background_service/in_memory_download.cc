// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/in_memory_download.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "components/download/internal/background_service/blob_task_proxy.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"

namespace download {

InMemoryDownload::InMemoryDownload(const std::string& guid)
    : guid_(guid),
      state_(State::INITIAL),
      paused_(false),
      bytes_downloaded_(0u) {}

InMemoryDownload::~InMemoryDownload() = default;

InMemoryDownloadImpl::InMemoryDownloadImpl(
    const std::string& guid,
    const RequestParams& request_params,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    Delegate* delegate,
    network::mojom::URLLoaderFactory* url_loader_factory,
    BlobTaskProxy::BlobContextGetter blob_context_getter,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : InMemoryDownload(guid),
      request_params_(request_params),
      traffic_annotation_(traffic_annotation),
      url_loader_factory_(url_loader_factory),
      blob_task_proxy_(
          BlobTaskProxy::Create(blob_context_getter, io_task_runner)),
      io_task_runner_(io_task_runner),
      delegate_(delegate),
      completion_notified_(false),
      weak_ptr_factory_(this) {
  DCHECK(!guid_.empty());
}

InMemoryDownloadImpl::~InMemoryDownloadImpl() {
  io_task_runner_->DeleteSoon(FROM_HERE, blob_task_proxy_.release());
}

void InMemoryDownloadImpl::Start() {
  DCHECK(state_ == State::INITIAL) << "Only call Start() for new download.";
  SendRequest();
  state_ = State::IN_PROGRESS;
}

void InMemoryDownloadImpl::Pause() {
  if (state_ == State::IN_PROGRESS)
    paused_ = true;
}

void InMemoryDownloadImpl::Resume() {
  paused_ = false;

  switch (state_) {
    case State::INITIAL:
      NOTREACHED();
      return;
    case State::IN_PROGRESS:
      // Let the network pipe continue to read data.
      if (resume_callback_)
        std::move(resume_callback_).Run();
      return;
    case State::FAILED:
      // Restart the download.
      Reset();
      SendRequest();
      state_ = State::IN_PROGRESS;
      return;
    case State::COMPLETE:
      NotifyDelegateDownloadComplete();
      return;
  }
}

std::unique_ptr<storage::BlobDataHandle> InMemoryDownloadImpl::ResultAsBlob()
    const {
  DCHECK(state_ == State::COMPLETE || state_ == State::FAILED);
  // Return a copy.
  return std::make_unique<storage::BlobDataHandle>(*blob_data_handle_);
}

size_t InMemoryDownloadImpl::EstimateMemoryUsage() const {
  return bytes_downloaded_;
}

void InMemoryDownloadImpl::OnDataReceived(base::StringPiece string_piece,
                                          base::OnceClosure resume) {
  size_t size = string_piece.as_string().size();
  data_.append(string_piece.as_string().data(), size);
  bytes_downloaded_ += size;

  if (paused_) {
    // Read data later and cache the resumption callback when paused.
    resume_callback_ = std::move(resume);
    return;
  }

  // Continue to read data.
  std::move(resume).Run();

  // TODO(xingliu): Throttle the update frequency. See https://crbug.com/809674.
  if (delegate_)
    delegate_->OnDownloadProgress(this);
}

void InMemoryDownloadImpl::OnComplete(bool success) {
  if (success) {
    SaveAsBlob();
    return;
  }

  state_ = State::FAILED;

  // Release download data.
  data_.clear();
  NotifyDelegateDownloadComplete();
}

void InMemoryDownloadImpl::OnRetry(base::OnceClosure start_retry) {
  Reset();
  std::move(start_retry).Run();
}

void InMemoryDownloadImpl::SaveAsBlob() {
  auto callback = base::BindOnce(&InMemoryDownloadImpl::OnSaveBlobDone,
                                 weak_ptr_factory_.GetWeakPtr());
  auto data = std::make_unique<std::string>(std::move(data_));
  blob_task_proxy_->SaveAsBlob(std::move(data), std::move(callback));
}

void InMemoryDownloadImpl::OnSaveBlobDone(
    std::unique_ptr<storage::BlobDataHandle> blob_handle,
    storage::BlobStatus status) {
  // |status| is valid on IO thread, consumer of |blob_handle| should validate
  // the data when using the blob data.
  state_ =
      (status == storage::BlobStatus::DONE) ? State::COMPLETE : State::FAILED;

  // TODO(xingliu): Add metric for blob status code. If failed, consider remove
  // |blob_data_handle_|. See https://crbug.com/809674.
  DCHECK(data_.empty())
      << "Download data should be contained in |blob_data_handle_|.";
  blob_data_handle_ = std::move(blob_handle);
  completion_time_ = base::Time::Now();

  // Resets network backend.
  loader_.reset();

  // Not considering |paused_| here, if pause after starting a blob operation,
  // just let it finish.
  NotifyDelegateDownloadComplete();
}

void InMemoryDownloadImpl::NotifyDelegateDownloadComplete() {
  if (completion_notified_)
    return;
  completion_notified_ = true;

  if (delegate_)
    delegate_->OnDownloadComplete(this);
}

void InMemoryDownloadImpl::SendRequest() {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = request_params_.url;
  request->method = request_params_.method;
  request->headers = request_params_.request_headers;
  request->load_flags = net::LOAD_DISABLE_CACHE;

  loader_ =
      network::SimpleURLLoader::Create(std::move(request), traffic_annotation_);

  // TODO(xingliu): Use SimpleURLLoader's retry when it won't hit CHECK in
  // SharedURLLoaderFactory.
  loader_->DownloadAsStream(url_loader_factory_, this);
}

void InMemoryDownloadImpl::Reset() {
  data_.clear();
  bytes_downloaded_ = 0u;
  completion_notified_ = false;
  resume_callback_.Reset();
}

}  // namespace download
