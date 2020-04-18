// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_delegate_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/offline_items_collection/offline_content_aggregator_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/download/public/background_service/download_params.h"
#include "components/download/public/background_service/download_service.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "components/offline_items_collection/core/offline_item.h"
#include "content/public/browser/background_fetch_description.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"

BackgroundFetchDelegateImpl::BackgroundFetchDelegateImpl(Profile* profile)
    : download_service_(
          DownloadServiceFactory::GetInstance()->GetForBrowserContext(profile)),
      offline_content_aggregator_(
          OfflineContentAggregatorFactory::GetForBrowserContext(profile)),
      weak_ptr_factory_(this) {
  offline_content_aggregator_->RegisterProvider("background_fetch", this);
}

BackgroundFetchDelegateImpl::~BackgroundFetchDelegateImpl() {}

void BackgroundFetchDelegateImpl::Shutdown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (client()) {
    client()->OnDelegateShutdown();
  }
}

BackgroundFetchDelegateImpl::JobDetails::JobDetails(JobDetails&&) = default;

BackgroundFetchDelegateImpl::JobDetails::JobDetails(
    std::unique_ptr<content::BackgroundFetchDescription> fetch_description)
    : cancelled(false),
      offline_item(offline_items_collection::ContentId(
          "background_fetch",
          fetch_description->job_unique_id)),
      fetch_description(std::move(fetch_description)) {
  UpdateOfflineItem();
}

BackgroundFetchDelegateImpl::JobDetails::~JobDetails() = default;

void BackgroundFetchDelegateImpl::JobDetails::UpdateOfflineItem() {
  DCHECK_GT(fetch_description->total_parts, 0);

  if (ShouldReportProgressBySize()) {
    offline_item.progress.value = fetch_description->completed_parts_size;
    // If we have completed all downloads, update progress max to
    // completed_parts_size in case total_parts_size was set too high. This
    // avoid unnecessary jumping in the progress bar.
    offline_item.progress.max =
        (fetch_description->completed_parts == fetch_description->total_parts)
            ? fetch_description->completed_parts_size
            : fetch_description->total_parts_size;
  } else {
    offline_item.progress.value = fetch_description->completed_parts;
    offline_item.progress.max = fetch_description->total_parts;
  }

  offline_item.progress.unit =
      offline_items_collection::OfflineItemProgressUnit::PERCENTAGE;

  if (fetch_description->title.empty()) {
    offline_item.title = fetch_description->origin.Serialize();
  } else {
    // TODO(crbug.com/774612): Make sure that the origin is displayed completely
    // in all cases so that long titles cannot obscure it.
    offline_item.title =
        base::StringPrintf("%s (%s)", fetch_description->title.c_str(),
                           fetch_description->origin.Serialize().c_str());
  }
  // TODO(delphick): Figure out what to put in offline_item.description.
  offline_item.is_transient = true;

  using OfflineItemState = offline_items_collection::OfflineItemState;
  if (cancelled)
    offline_item.state = OfflineItemState::CANCELLED;
  else if (fetch_description->completed_parts == fetch_description->total_parts)
    offline_item.state = OfflineItemState::COMPLETE;
  else
    offline_item.state = OfflineItemState::IN_PROGRESS;
}

bool BackgroundFetchDelegateImpl::JobDetails::ShouldReportProgressBySize() {
  if (!fetch_description->total_parts_size) {
    // total_parts_size was not set. Cannot report by size.
    return false;
  }

  if (fetch_description->completed_parts < fetch_description->total_parts &&
      fetch_description->completed_parts_size >
          fetch_description->total_parts_size) {
    // total_parts_size was set too low.
    return false;
  }

  return true;
}

void BackgroundFetchDelegateImpl::GetIconDisplaySize(
    BackgroundFetchDelegate::GetIconDisplaySizeCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // If Android, return 192x192, else return 0x0. 0x0 means not loading an
  // icon at all, which is returned for all non-Android platforms as the
  // icons can't be displayed on the UI yet.
  // TODO(nator): Move this logic to OfflineItemsCollection, and return icon
  // size based on display.
  gfx::Size display_size;
#if defined(OS_ANDROID)
  display_size = gfx::Size(192, 192);
#endif
  std::move(callback).Run(display_size);
}

void BackgroundFetchDelegateImpl::CreateDownloadJob(
    std::unique_ptr<content::BackgroundFetchDescription> fetch_description) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string job_unique_id = fetch_description->job_unique_id;
  DCHECK(!job_details_map_.count(job_unique_id));
  auto emplace_result = job_details_map_.emplace(
      job_unique_id, JobDetails(std::move(fetch_description)));

  const JobDetails& details = emplace_result.first->second;
  for (const auto& download_guid : details.fetch_description->current_guids) {
    DCHECK(!download_job_unique_id_map_.count(download_guid));
    download_job_unique_id_map_.emplace(download_guid, job_unique_id);
  }

  for (auto* observer : observers_) {
    observer->OnItemsAdded({details.offline_item});
  }
}

void BackgroundFetchDelegateImpl::DownloadUrl(
    const std::string& job_unique_id,
    const std::string& download_guid,
    const std::string& method,
    const GURL& url,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    const net::HttpRequestHeaders& headers) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(job_details_map_.count(job_unique_id));
  DCHECK(!download_job_unique_id_map_.count(download_guid));

  JobDetails& job_details = job_details_map_.find(job_unique_id)->second;
  job_details.current_download_guids.insert(download_guid);

  download_job_unique_id_map_.emplace(download_guid, job_unique_id);

  download::DownloadParams params;
  params.guid = download_guid;
  params.client = download::DownloadClient::BACKGROUND_FETCH;
  params.request_params.method = method;
  params.request_params.url = url;
  params.request_params.request_headers = headers;
  params.callback = base::Bind(&BackgroundFetchDelegateImpl::OnDownloadReceived,
                               weak_ptr_factory_.GetWeakPtr());
  params.traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(traffic_annotation);

  download_service_->StartDownload(params);
}

void BackgroundFetchDelegateImpl::Abort(const std::string& job_unique_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto job_details_iter = job_details_map_.find(job_unique_id);
  if (job_details_iter == job_details_map_.end())
    return;

  JobDetails& job_details = job_details_iter->second;
  job_details.cancelled = true;

  for (const auto& download_guid : job_details.current_download_guids) {
    download_service_->CancelDownload(download_guid);
    download_job_unique_id_map_.erase(download_guid);
  }
  UpdateOfflineItemAndUpdateObservers(&job_details);
  job_details_map_.erase(job_details_iter);
}

void BackgroundFetchDelegateImpl::OnDownloadStarted(
    const std::string& download_guid,
    std::unique_ptr<content::BackgroundFetchResponse> response) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto download_job_unique_id_iter =
      download_job_unique_id_map_.find(download_guid);
  // TODO(crbug.com/779012): When DownloadService fixes cancelled jobs calling
  // OnDownload* methods, then this can be a DCHECK.
  if (download_job_unique_id_iter == download_job_unique_id_map_.end())
    return;

  const std::string& job_unique_id = download_job_unique_id_iter->second;

  if (client()) {
    client()->OnDownloadStarted(job_unique_id, download_guid,
                                std::move(response));
  }
}

void BackgroundFetchDelegateImpl::OnDownloadUpdated(
    const std::string& download_guid,
    uint64_t bytes_downloaded) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto download_job_unique_id_iter =
      download_job_unique_id_map_.find(download_guid);
  // TODO(crbug.com/779012): When DownloadService fixes cancelled jobs calling
  // OnDownload* methods, then this can be a DCHECK.
  if (download_job_unique_id_iter == download_job_unique_id_map_.end())
    return;

  const std::string& job_unique_id = download_job_unique_id_iter->second;

  // This will update the progress bar.
  DCHECK(job_details_map_.count(job_unique_id));
  JobDetails& job_details = job_details_map_.find(job_unique_id)->second;
  job_details.fetch_description->completed_parts_size = bytes_downloaded;
  if (job_details.fetch_description->total_parts_size &&
      job_details.fetch_description->total_parts_size <
          job_details.fetch_description->completed_parts_size) {
    // Fail the fetch if total download size was set too low.
    // We only do this if total download size is specified. If not specified,
    // this check is skipped. This is to allow for situations when the
    // total download size cannot be known when invoking fetch.
    FailFetch(job_unique_id);
    return;
  }
  UpdateOfflineItemAndUpdateObservers(&job_details);

  if (client())
    client()->OnDownloadUpdated(job_unique_id, download_guid, bytes_downloaded);
}

void BackgroundFetchDelegateImpl::OnDownloadFailed(
    const std::string& download_guid,
    download::Client::FailureReason reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  using FailureReason = content::BackgroundFetchResult::FailureReason;
  FailureReason failure_reason;

  auto download_job_unique_id_iter =
      download_job_unique_id_map_.find(download_guid);
  // TODO(crbug.com/779012): When DownloadService fixes cancelled jobs
  // potentially calling OnDownloadFailed with a reason other than
  // CANCELLED/ABORTED, we should add a DCHECK here.
  if (download_job_unique_id_iter == download_job_unique_id_map_.end())
    return;

  const std::string& job_unique_id = download_job_unique_id_iter->second;
  JobDetails& job_details = job_details_map_.find(job_unique_id)->second;
  ++job_details.fetch_description->completed_parts;
  UpdateOfflineItemAndUpdateObservers(&job_details);

  switch (reason) {
    case download::Client::FailureReason::NETWORK:
      failure_reason = FailureReason::NETWORK;
      break;
    case download::Client::FailureReason::TIMEDOUT:
      failure_reason = FailureReason::TIMEDOUT;
      break;
    case download::Client::FailureReason::UNKNOWN:
      failure_reason = FailureReason::UNKNOWN;
      break;

    case download::Client::FailureReason::ABORTED:
    case download::Client::FailureReason::CANCELLED:
      // The client cancelled or aborted it so no need to notify it.
      return;
    default:
      NOTREACHED();
      return;
  }

  // TODO(delphick): consider calling OnItemUpdated here as well if for instance
  // the download actually happened but 404ed.

  if (client()) {
    client()->OnDownloadComplete(
        job_unique_id, download_guid,
        std::make_unique<content::BackgroundFetchResult>(base::Time::Now(),
                                                         failure_reason));
  }

  job_details.current_download_guids.erase(
      job_details.current_download_guids.find(download_guid));
  download_job_unique_id_map_.erase(download_guid);
}

void BackgroundFetchDelegateImpl::OnDownloadSucceeded(
    const std::string& download_guid,
    const base::FilePath& path,
    uint64_t size) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto download_job_unique_id_iter =
      download_job_unique_id_map_.find(download_guid);
  // TODO(crbug.com/779012): When DownloadService fixes cancelled jobs calling
  // OnDownload* methods, then this can be a DCHECK.
  if (download_job_unique_id_iter == download_job_unique_id_map_.end())
    return;

  const std::string& job_unique_id = download_job_unique_id_iter->second;
  JobDetails& job_details = job_details_map_.find(job_unique_id)->second;
  ++job_details.fetch_description->completed_parts;
  job_details.fetch_description->completed_parts_size = size;
  UpdateOfflineItemAndUpdateObservers(&job_details);

  if (client()) {
    client()->OnDownloadComplete(
        job_unique_id, download_guid,
        std::make_unique<content::BackgroundFetchResult>(base::Time::Now(),
                                                         path, size));
  }

  job_details.current_download_guids.erase(
      job_details.current_download_guids.find(download_guid));
  download_job_unique_id_map_.erase(download_guid);
}

void BackgroundFetchDelegateImpl::OnDownloadReceived(
    const std::string& download_guid,
    download::DownloadParams::StartResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  using StartResult = download::DownloadParams::StartResult;
  switch (result) {
    case StartResult::ACCEPTED:
      // Nothing to do.
      break;
    case StartResult::BACKOFF:
      // TODO(delphick): try again later?
      NOTREACHED();
      break;
    case StartResult::UNEXPECTED_CLIENT:
      // This really should never happen since we're supplying the
      // DownloadClient.
      NOTREACHED();
      break;
    case StartResult::UNEXPECTED_GUID:
      // TODO(delphick): try again with a different GUID.
      NOTREACHED();
      break;
    case StartResult::CLIENT_CANCELLED:
      // TODO(delphick): do we need to do anything here, since we will have
      // cancelled it?
      break;
    case StartResult::INTERNAL_ERROR:
      // TODO(delphick): We need to handle this gracefully.
      NOTREACHED();
      break;
    case StartResult::COUNT:
      NOTREACHED();
      break;
  }
}

// Much of the code in offline_item_collection is not re-entrant, so this should
// not be called from any of the OfflineContentProvider-inherited methods.
void BackgroundFetchDelegateImpl::UpdateOfflineItemAndUpdateObservers(
    JobDetails* job_details) {
  job_details->UpdateOfflineItem();

  for (auto* observer : observers_)
    observer->OnItemUpdated(job_details->offline_item);
}

void BackgroundFetchDelegateImpl::OpenItem(
    const offline_items_collection::ContentId& id) {
  // TODO(delphick): Add custom OpenItem behavior.
  NOTIMPLEMENTED();
}

void BackgroundFetchDelegateImpl::RemoveItem(
    const offline_items_collection::ContentId& id) {
  // TODO(delphick): Support removing items. (Not sure when this would actually
  // get called though).
  NOTIMPLEMENTED();
}

void BackgroundFetchDelegateImpl::FailFetch(const std::string& job_unique_id) {
  // Save a copy before Abort() deletes the reference.
  const std::string unique_id = job_unique_id;
  Abort(job_unique_id);
  if (client()) {
    client()->OnJobCancelled(
        unique_id,
        content::BackgroundFetchReasonToAbort::TOTAL_DOWNLOAD_SIZE_EXCEEDED);
  }
}

void BackgroundFetchDelegateImpl::CancelDownload(
    const offline_items_collection::ContentId& id) {
  Abort(id.id);

  if (client()) {
    client()->OnJobCancelled(
        id.id, content::BackgroundFetchReasonToAbort::CANCELLED_FROM_UI);
  }
}

void BackgroundFetchDelegateImpl::PauseDownload(
    const offline_items_collection::ContentId& id) {
  auto job_details_iter = job_details_map_.find(id.id);
  if (job_details_iter == job_details_map_.end())
    return;

  JobDetails& job_details = job_details_iter->second;
  for (auto& download_guid : job_details.current_download_guids)
    download_service_->PauseDownload(download_guid);

  // TODO(delphick): Mark overall download job as paused so that future
  // downloads are not started until resume. (Initially not a worry because only
  // one download will be scheduled at a time).
}

void BackgroundFetchDelegateImpl::ResumeDownload(
    const offline_items_collection::ContentId& id,
    bool has_user_gesture) {
  auto job_details_iter = job_details_map_.find(id.id);
  if (job_details_iter == job_details_map_.end())
    return;

  JobDetails& job_details = job_details_iter->second;
  for (auto& download_guid : job_details.current_download_guids)
    download_service_->ResumeDownload(download_guid);

  // TODO(delphick): Start new downloads that weren't started because of pause.
}

void BackgroundFetchDelegateImpl::GetItemById(
    const offline_items_collection::ContentId& id,
    SingleItemCallback callback) {
  auto it = job_details_map_.find(id.id);
  base::Optional<offline_items_collection::OfflineItem> offline_item;
  if (it != job_details_map_.end())
    offline_item = it->second.offline_item;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), offline_item));
}

void BackgroundFetchDelegateImpl::GetAllItems(MultipleItemCallback callback) {
  OfflineItemList item_list;
  for (auto& entry : job_details_map_)
    item_list.push_back(entry.second.offline_item);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), item_list));
}

void BackgroundFetchDelegateImpl::GetVisualsForItem(
    const offline_items_collection::ContentId& id,
    const VisualsCallback& callback) {
  // GetVisualsForItem mustn't be called directly since offline_items_collection
  // is not re-entrant and it must be called even if there are no visuals.
  auto visuals =
      std::make_unique<offline_items_collection::OfflineItemVisuals>();
  auto it = job_details_map_.find(id.id);
  if (it != job_details_map_.end()) {
    visuals->icon =
        gfx::Image::CreateFrom1xBitmap(it->second.fetch_description->icon);
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback, id, std::move(visuals)));
}

void BackgroundFetchDelegateImpl::AddObserver(Observer* observer) {
  DCHECK(!observers_.count(observer));

  observers_.insert(observer);
}

void BackgroundFetchDelegateImpl::RemoveObserver(Observer* observer) {
  observers_.erase(observer);
}
