// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_download_client.h"

#include <memory>
#include <utility>

#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/background_fetch/background_fetch_delegate_impl.h"
#include "chrome/browser/download/download_service_factory.h"
#include "components/download/public/background_service/download_metadata.h"
#include "components/download/public/background_service/download_service.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_context.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "url/origin.h"

BackgroundFetchDownloadClient::BackgroundFetchDownloadClient(
    content::BrowserContext* context)
    : browser_context_(context), delegate_(nullptr) {}

BackgroundFetchDownloadClient::~BackgroundFetchDownloadClient() = default;

void BackgroundFetchDownloadClient::OnServiceInitialized(
    bool state_lost,
    const std::vector<download::DownloadMetaData>& downloads) {
  content::BackgroundFetchDelegate* delegate =
      browser_context_->GetBackgroundFetchDelegate();

  // TODO(crbug.com/766082): Support incognito mode in
  // BackgroundFetchDelegateFactory, currently |delegate| will be nullptr in
  // incognito mode.
  if (!delegate)
    return;

  delegate_ = static_cast<BackgroundFetchDelegateImpl*>(
                  browser_context_->GetBackgroundFetchDelegate())
                  ->GetWeakPtr();
  DCHECK(delegate_);

  // TODO(delphick): Reconnect the outstanding downloads with the content layer
  // part of background_fetch. For now we just cancel all the downloads.
  if (downloads.size() > 0) {
    download::DownloadService* download_service =
        DownloadServiceFactory::GetInstance()->GetForBrowserContext(
            browser_context_);
    for (const auto& download : downloads)
      download_service->CancelDownload(download.guid);
  }
}

void BackgroundFetchDownloadClient::OnServiceUnavailable() {}

download::Client::ShouldDownload
BackgroundFetchDownloadClient::OnDownloadStarted(
    const std::string& guid,
    const std::vector<GURL>& url_chain,
    const scoped_refptr<const net::HttpResponseHeaders>& headers) {
  if (delegate_) {
    std::unique_ptr<content::BackgroundFetchResponse> response =
        std::make_unique<content::BackgroundFetchResponse>(url_chain, headers);
    delegate_->OnDownloadStarted(guid, std::move(response));
  }

  // TODO(delphick): validate the chain/headers before returning CONTINUE
  return download::Client::ShouldDownload::CONTINUE;
}

void BackgroundFetchDownloadClient::OnDownloadUpdated(
    const std::string& guid,
    uint64_t bytes_downloaded) {
  if (delegate_)
    delegate_->OnDownloadUpdated(guid, bytes_downloaded);
}

void BackgroundFetchDownloadClient::OnDownloadFailed(
    const std::string& guid,
    download::Client::FailureReason reason) {
  if (delegate_)
    delegate_->OnDownloadFailed(guid, reason);
}

void BackgroundFetchDownloadClient::OnDownloadSucceeded(
    const std::string& guid,
    const download::CompletionInfo& info) {
  if (delegate_)
    delegate_->OnDownloadSucceeded(guid, info.path, info.bytes_downloaded);
}

bool BackgroundFetchDownloadClient::CanServiceRemoveDownloadedFile(
    const std::string& guid,
    bool force_delete) {
  // TODO(delphick): Return false if the background fetch hasn't finished yet
  return true;
}

void BackgroundFetchDownloadClient::GetUploadData(
    const std::string& guid,
    download::GetUploadDataCallback callback) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), nullptr));
}
