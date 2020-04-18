// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/downloads/download_ui_adapter.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/downloads/offline_item_conversions.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/thumbnail_decoder.h"
#include "ui/gfx/image/image.h"

namespace {
// Value of this constant doesn't matter, only its address is used.
const char kDownloadUIAdapterKey[] = "";
}  // namespace

namespace offline_pages {

namespace {

std::vector<int64_t> FilterRequestsByGuid(
    std::vector<std::unique_ptr<SavePageRequest>> requests,
    const std::string& guid,
    ClientPolicyController* policy_controller) {
  std::vector<int64_t> request_ids;
  for (const auto& request : requests) {
    if (request->client_id().id == guid &&
        policy_controller->IsSupportedByDownload(
            request->client_id().name_space)) {
      request_ids.push_back(request->request_id());
    }
  }
  return request_ids;
}

}  // namespace

// static
DownloadUIAdapter* DownloadUIAdapter::FromOfflinePageModel(
    OfflinePageModel* model) {
  DCHECK(model);
  return static_cast<DownloadUIAdapter*>(
      model->GetUserData(kDownloadUIAdapterKey));
}

// static
void DownloadUIAdapter::AttachToOfflinePageModel(
    std::unique_ptr<DownloadUIAdapter> adapter,
    OfflinePageModel* model) {
  DCHECK(adapter);
  DCHECK(model);
  model->SetUserData(kDownloadUIAdapterKey, std::move(adapter));
}

DownloadUIAdapter::ItemInfo::ItemInfo(const OfflinePageItem& page,
                                      bool temporarily_hidden,
                                      bool is_suggested)
    : ui_item(std::make_unique<OfflineItem>(
          OfflineItemConversions::CreateOfflineItem(page, is_suggested))),
      is_request(false),
      offline_id(page.offline_id),
      client_id(page.client_id),
      temporarily_hidden(temporarily_hidden) {}

DownloadUIAdapter::ItemInfo::ItemInfo(const SavePageRequest& request,
                                      bool temporarily_hidden)
    : ui_item(std::make_unique<OfflineItem>(
          OfflineItemConversions::CreateOfflineItem(request))),
      is_request(true),
      offline_id(request.request_id()),
      client_id(request.client_id()),
      temporarily_hidden() {}

DownloadUIAdapter::ItemInfo::~ItemInfo() {}

DownloadUIAdapter::DownloadUIAdapter(
    OfflineContentAggregator* aggregator,
    OfflinePageModel* model,
    RequestCoordinator* request_coordinator,
    std::unique_ptr<ThumbnailDecoder> thumbnail_decoder,
    std::unique_ptr<Delegate> delegate)
    : aggregator_(aggregator),
      model_(model),
      request_coordinator_(request_coordinator),
      thumbnail_decoder_(std::move(thumbnail_decoder)),
      delegate_(std::move(delegate)),
      state_(State::NOT_LOADED),
      weak_ptr_factory_(this) {
  delegate_->SetUIAdapter(this);
  if (aggregator_)
    aggregator_->RegisterProvider(kOfflinePageNamespace, this);
}

DownloadUIAdapter::~DownloadUIAdapter() {
  if (aggregator_)
    aggregator_->UnregisterProvider(kOfflinePageNamespace);
}

void DownloadUIAdapter::AddObserver(
    OfflineContentProvider::Observer* observer) {
  DCHECK(observer);
  if (observers_.HasObserver(observer))
    return;
  observers_.AddObserver(observer);
}

void DownloadUIAdapter::RemoveObserver(
    OfflineContentProvider::Observer* observer) {
  DCHECK(observer);
  if (!observers_.HasObserver(observer))
    return;
  observers_.RemoveObserver(observer);
}

void DownloadUIAdapter::OfflinePageModelLoaded(OfflinePageModel* model) {
  // This signal is not used here.
}

void DownloadUIAdapter::OfflinePageAdded(OfflinePageModel* model,
                                         const OfflinePageItem& added_page) {
  DCHECK(model == model_);
  if (!delegate_->IsVisibleInUI(added_page.client_id))
    return;

  bool temporarily_hidden =
      delegate_->IsTemporarilyHiddenInUI(added_page.client_id);
  bool is_suggested = model->GetPolicyController()->IsSuggested(
      added_page.client_id.name_space);
  AddItemHelper(
      std::make_unique<ItemInfo>(added_page, temporarily_hidden, is_suggested));
}

void DownloadUIAdapter::OfflinePageDeleted(
    const OfflinePageModel::DeletedPageInfo& page_info) {
  if (!delegate_->IsVisibleInUI(page_info.client_id))
    return;
  DeleteItemHelper(page_info.client_id.id);
}

// RequestCoordinator::Observer
void DownloadUIAdapter::OnAdded(const SavePageRequest& added_request) {
  if (!delegate_->IsVisibleInUI(added_request.client_id()))
    return;

  bool temporarily_hidden =
      delegate_->IsTemporarilyHiddenInUI(added_request.client_id());
  AddItemHelper(std::make_unique<ItemInfo>(added_request, temporarily_hidden));
}

// RequestCoordinator::Observer
void DownloadUIAdapter::OnCompleted(
    const SavePageRequest& request,
    RequestNotifier::BackgroundSavePageResult status) {
  if (!delegate_->IsVisibleInUI(request.client_id()))
    return;

  // If request completed successfully, report ItemUpdated when a page is added
  // to the model. If the request failed, tell UI that the item is gone.
  if (status == RequestNotifier::BackgroundSavePageResult::SUCCESS)
    return;
  DeleteItemHelper(request.client_id().id);
}

// RequestCoordinator::Observer
void DownloadUIAdapter::OnChanged(const SavePageRequest& request) {
  if (!delegate_->IsVisibleInUI(request.client_id()))
    return;

  std::string guid = request.client_id().id;

  // There is a chance that when OnChanged comes from RequestCoordinator,
  // the item has already been downloaded and this update would cause an
  // incorrect "in progress" state to be shown in UI.
  bool page_already_added =
      items_.find(guid) != items_.end() && !items_[guid]->is_request;
  if (page_already_added)
    return;

  bool temporarily_hidden =
      delegate_->IsTemporarilyHiddenInUI(request.client_id());
  items_[guid] = std::make_unique<ItemInfo>(request, temporarily_hidden);

  if (state_ != State::LOADED)
    return;

  const OfflineItem& offline_item = *(items_[guid]->ui_item);
  for (OfflineContentProvider::Observer& observer : observers_)
    observer.OnItemUpdated(offline_item);
}

void DownloadUIAdapter::OnNetworkProgress(const SavePageRequest& request,
                                          int64_t received_bytes) {
  if (state_ != State::LOADED)
    return;

  for (auto& item : items_) {
    if (item.second->is_request &&
        item.second->offline_id == request.request_id()) {
      if (received_bytes == item.second->ui_item->received_bytes)
        return;

      item.second->ui_item->received_bytes = received_bytes;
      for (auto& observer : observers_)
        observer.OnItemUpdated(*(item.second->ui_item));
      return;
    }
  }
}

void DownloadUIAdapter::TemporaryHiddenStatusChanged(
    const ClientId& client_id) {
  if (state_ != State::LOADED)
    return;

  bool hidden = delegate_->IsTemporarilyHiddenInUI(client_id);

  for (const auto& item : items_) {
    if (item.second->client_id == client_id) {
      if (item.second->temporarily_hidden == hidden)
        continue;
      item.second->temporarily_hidden = hidden;
      if (hidden) {
        for (auto& observer : observers_)
          observer.OnItemRemoved(item.second->ui_item->id);
      } else {
        for (auto& observer : observers_) {
          observer.OnItemsAdded({*item.second->ui_item});
        }
      }
    }
  }
}

void DownloadUIAdapter::GetAllItems(
    OfflineContentProvider::MultipleItemCallback callback) {
  if (state_ == State::LOADED) {
    ReplyWithAllItems(std::move(callback));
    return;
  }

  postponed_callbacks_.emplace_back(std::move(callback));
  LoadCache();
}

void DownloadUIAdapter::GetVisualsForItem(
    const ContentId& id,
    const VisualsCallback& visuals_callback) {
  auto it = items_.find(id.id);
  if (it == items_.end() || !thumbnail_decoder_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(visuals_callback, id, nullptr));
    return;
  }
  const ItemInfo* item = it->second.get();

  VisualResultCallback callback = base::BindOnce(visuals_callback, id);
  if (item->client_id.name_space == kSuggestedArticlesNamespace) {
    // Report PrefetchedItemHasThumbnail along with result callback.
    auto report_and_callback =
        [](VisualResultCallback result_callback,
           std::unique_ptr<offline_items_collection::OfflineItemVisuals>
               visuals) {
          UMA_HISTOGRAM_BOOLEAN(
              "OfflinePages.DownloadUI.PrefetchedItemHasThumbnail",
              visuals != nullptr);
          std::move(result_callback).Run(std::move(visuals));
        };
    callback = base::BindOnce(report_and_callback, std::move(callback));
  }

  model_->GetThumbnailByOfflineId(
      item->offline_id,
      base::BindOnce(&DownloadUIAdapter::OnThumbnailLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void DownloadUIAdapter::OnThumbnailLoaded(
    VisualResultCallback callback,
    std::unique_ptr<OfflinePageThumbnail> thumbnail) {
  DCHECK(thumbnail_decoder_);
  if (!thumbnail || thumbnail->thumbnail.empty()) {
    // PostTask not required, GetThumbnailByOfflineId does it for us.
    std::move(callback).Run(nullptr);
    return;
  }

  auto forward_visuals_lambda = [](VisualResultCallback callback,
                                   const gfx::Image& image) {
    if (image.IsEmpty()) {
      std::move(callback).Run(nullptr);
      return;
    }
    auto visuals =
        std::make_unique<offline_items_collection::OfflineItemVisuals>();
    visuals->icon = image;
    std::move(callback).Run(std::move(visuals));
  };

  thumbnail_decoder_->DecodeAndCropThumbnail(
      thumbnail->thumbnail,
      base::BindOnce(forward_visuals_lambda, std::move(callback)));
}

void DownloadUIAdapter::ThumbnailAdded(OfflinePageModel* model,
                                       const OfflinePageThumbnail& thumbnail) {
  // Note, this is an O(N) lookup. Not ideal, but this method is called at most
  // 10 times a day (once per prefetch download), so it's probably not worth
  // optimizing.
  auto it =
      std::find_if(items_.cbegin(), items_.cend(),
                   [&](const OfflineItems::value_type& entry) {
                     return entry.second->offline_id == thumbnail.offline_id;
                   });
  if (it == items_.end() || !it->second->ui_item)
    return;
  for (auto& observer : observers_)
    observer.OnItemUpdated(*it->second->ui_item);
}

// TODO(dimich): Remove this method since it is not used currently. If needed,
// it has to be updated to fault in the initial load of items. Currently it
// simply returns nullopt if the cache is not loaded.
void DownloadUIAdapter::GetItemById(
    const ContentId& id,
    OfflineContentProvider::SingleItemCallback callback) {
  base::Optional<OfflineItem> offline_item;
  if (state_ == State::LOADED) {
    OfflineItems::const_iterator it = items_.find(id.id);
    if (it != items_.end() && it->second->ui_item &&
        !delegate_->IsTemporarilyHiddenInUI(it->second->client_id)) {
      offline_item = *it->second->ui_item;
    }
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), offline_item));
}

void DownloadUIAdapter::OpenItem(const ContentId& id) {
  if (state_ == State::LOADED) {
    OpenItemByGuid(id.id);
    return;
  }

  postponed_operations_.push_back(
      base::BindOnce(&DownloadUIAdapter::OpenItemByGuid,
                     weak_ptr_factory_.GetWeakPtr(), id.id));
  LoadCache();
}

void DownloadUIAdapter::RemoveItem(const ContentId& id) {
  if (state_ == State::LOADED) {
    RemoveItemByGuid(id.id);
    return;
  }

  postponed_operations_.push_back(
      base::BindOnce(&DownloadUIAdapter::RemoveItemByGuid,
                     weak_ptr_factory_.GetWeakPtr(), id.id));
  LoadCache();
}

int64_t DownloadUIAdapter::GetOfflineIdByGuid(const std::string& guid) const {
  if (state_ != State::LOADED)
    return 0;

  if (deleting_item_ && deleting_item_->ui_item->id.id == guid)
    return deleting_item_->offline_id;

  OfflineItems::const_iterator it = items_.find(guid);
  if (it != items_.end())
    return it->second->offline_id;
  return 0;
}

void DownloadUIAdapter::CancelDownload(const ContentId& id) {
  // TODO(fgorski): Clean this up in a way where 2 round trips + GetAllRequests
  // is not necessary. E.g. CancelByGuid(guid) might do the trick.
  request_coordinator_->GetAllRequests(
      base::BindOnce(&DownloadUIAdapter::CancelDownloadContinuation,
                     weak_ptr_factory_.GetWeakPtr(), id.id));
}

void DownloadUIAdapter::CancelDownloadContinuation(
    const std::string& guid,
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  std::vector<int64_t> request_ids = FilterRequestsByGuid(
      std::move(requests), guid, request_coordinator_->GetPolicyController());
  request_coordinator_->RemoveRequests(request_ids, base::DoNothing());
}

void DownloadUIAdapter::PauseDownload(const ContentId& id) {
  // TODO(fgorski): Clean this up in a way where 2 round trips + GetAllRequests
  // is not necessary.
  request_coordinator_->GetAllRequests(
      base::Bind(&DownloadUIAdapter::PauseDownloadContinuation,
                 weak_ptr_factory_.GetWeakPtr(), id.id));
}

void DownloadUIAdapter::PauseDownloadContinuation(
    const std::string& guid,
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  request_coordinator_->PauseRequests(FilterRequestsByGuid(
      std::move(requests), guid, request_coordinator_->GetPolicyController()));
}

void DownloadUIAdapter::ResumeDownload(const ContentId& id,
                                       bool has_user_gesture) {
  // TODO(fgorski): Clean this up in a way where 2 round trips + GetAllRequests
  // is not necessary.
  if (has_user_gesture) {
    request_coordinator_->GetAllRequests(
        base::Bind(&DownloadUIAdapter::ResumeDownloadContinuation,
                   weak_ptr_factory_.GetWeakPtr(), id.id));
  } else {
    request_coordinator_->StartImmediateProcessing(base::DoNothing());
  }
}

void DownloadUIAdapter::ResumeDownloadContinuation(
    const std::string& guid,
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  request_coordinator_->ResumeRequests(FilterRequestsByGuid(
      std::move(requests), guid, request_coordinator_->GetPolicyController()));
}

// Note that several LoadCache calls may be issued before the async GetAllPages
// comes back.
void DownloadUIAdapter::LoadCache() {
  if (state_ != State::NOT_LOADED)
    return;
  state_ = State::LOADING_PAGES;
  model_->GetAllPages(base::BindOnce(&DownloadUIAdapter::OnOfflinePagesLoaded,
                                     weak_ptr_factory_.GetWeakPtr()));
}

// TODO(dimich): Start clearing this cache on UI close. Also, after OpenItem can
// done without loading all items from database.
void DownloadUIAdapter::ClearCache() {
  // Once loaded, this class starts to observe the model. Only remove observer
  // if it was added.
  if (state_ == State::LOADED) {
    model_->RemoveObserver(this);
    request_coordinator_->RemoveObserver(this);
  }
  items_.clear();
  state_ = State::NOT_LOADED;
  TRACE_EVENT_ASYNC_END0("offline_pages", "DownloadUIAdapter: items cached",
                         this);
}

void DownloadUIAdapter::OnOfflinePagesLoaded(
    const MultipleOfflinePageItemResult& pages) {
  // If multiple observers register quickly, the cache might be already loaded
  // by the previous LoadCache call. At the same time, if all observers already
  // left, there is no reason to populate the cache.
  if (state_ != State::LOADING_PAGES)
    return;
  for (const auto& page : pages) {
    if (delegate_->IsVisibleInUI(page.client_id)) {
      std::string guid = page.client_id.id;
      DCHECK(items_.find(guid) == items_.end());
      bool temporarily_hidden =
          delegate_->IsTemporarilyHiddenInUI(page.client_id);
      bool is_suggested =
          model_->GetPolicyController()->IsSuggested(page.client_id.name_space);
      std::unique_ptr<ItemInfo> item =
          std::make_unique<ItemInfo>(page, temporarily_hidden, is_suggested);
      items_[guid] = std::move(item);
    }
  }
  model_->AddObserver(this);

  state_ = State::LOADING_REQUESTS;
  request_coordinator_->GetAllRequests(base::Bind(
      &DownloadUIAdapter::OnRequestsLoaded, weak_ptr_factory_.GetWeakPtr()));
}

void DownloadUIAdapter::OnRequestsLoaded(
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  // If multiple observers register quickly, the cache might be already loaded
  // by the previous LoadCache call. At the same time, if all observers already
  // left, there is no reason to populate the cache.
  if (state_ != State::LOADING_REQUESTS)
    return;

  for (const auto& request : requests) {
    if (delegate_->IsVisibleInUI(request->client_id())) {
      std::string guid = request->client_id().id;
      DCHECK(items_.find(guid) == items_.end());
      bool temporarily_hidden =
          delegate_->IsTemporarilyHiddenInUI(request->client_id());
      std::unique_ptr<ItemInfo> item =
          std::make_unique<ItemInfo>(*request, temporarily_hidden);
      items_[guid] = std::move(item);
    }
  }
  request_coordinator_->AddObserver(this);

  state_ = State::LOADED;
  TRACE_EVENT_ASYNC_BEGIN1("offline_pages", "DownloadUIAdapter: items cached",
                           this, "initial count", items_.size());

  // If there are callers waiting for GetAllItems callback, call them.
  for (auto& callback : postponed_callbacks_) {
    ReplyWithAllItems(std::move(callback));
  }
  postponed_callbacks_.clear();

  // If there were requests to perform operations on items before cache was
  // loaded, perform them now.
  for (auto& operation : postponed_operations_) {
    std::move(operation).Run();
  }
  postponed_operations_.clear();
}

void DownloadUIAdapter::OnDeletePagesDone(DeletePageResult result) {
  // TODO(dimich): Consider adding UMA to record user actions.
}

void DownloadUIAdapter::AddItemHelper(std::unique_ptr<ItemInfo> item_info) {
  const std::string& guid = item_info->ui_item->id.id;

  OfflineItems::const_iterator it = items_.find(guid);
  // In case when request is completed and morphed into a page, this comes as
  // new page added and request completed. We ignore request completion
  // notification and when page is added, fire 'updated' instead of 'added'.
  bool request_to_page_transition =
      (it != items_.end() && it->second->is_request && !item_info->is_request);

  items_[guid] = std::move(item_info);

  if (state_ != State::LOADED)
    return;

  OfflineItem* offline_item = items_[guid]->ui_item.get();

  if (request_to_page_transition) {
    offline_item->state = offline_items_collection::OfflineItemState::COMPLETE;
    offline_item->progress.value = 100;
    offline_item->progress.max = 100L;
    offline_item->progress.unit =
        offline_items_collection::OfflineItemProgressUnit::PERCENTAGE;
    if (!items_[guid]->temporarily_hidden) {
      for (auto& observer : observers_)
        observer.OnItemUpdated(*offline_item);
    }
  } else {
    if (!items_[guid]->temporarily_hidden) {
      std::vector<OfflineItem> items(1, *offline_item);
      for (auto& observer : observers_)
        observer.OnItemsAdded(items);
    }
  }
}

void DownloadUIAdapter::DeleteItemHelper(const std::string& guid) {
  OfflineItems::iterator it = items_.find(guid);
  if (it == items_.end())
    return;
  DCHECK(deleting_item_ == nullptr);
  deleting_item_ = std::move(it->second);
  items_.erase(it);

  if (!deleting_item_->temporarily_hidden && state_ == State::LOADED) {
    for (auto& observer : observers_)
      observer.OnItemRemoved(ContentId(kOfflinePageNamespace, guid));
  }

  deleting_item_.reset();
}

void DownloadUIAdapter::ReplyWithAllItems(
    OfflineContentProvider::MultipleItemCallback callback) {
  std::vector<OfflineItem> items;
  for (const auto& item : items_) {
    if (delegate_->IsTemporarilyHiddenInUI(item.second->client_id))
      continue;
    items.push_back(*(item.second->ui_item));
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), items));
}

void DownloadUIAdapter::OpenItemByGuid(const std::string& guid) {
  if (state_ != State::LOADED) {
    return;
  }

  OfflineItems::const_iterator it = items_.find(guid);
  if (it == items_.end())
    return;

  const OfflineItem* item = it->second->ui_item.get();
  if (!item)
    return;

  delegate_->OpenItem(*item, GetOfflineIdByGuid(guid));
}

void DownloadUIAdapter::RemoveItemByGuid(const std::string& guid) {
  if (state_ != State::LOADED) {
    return;
  }

  OfflineItems::const_iterator it = items_.find(guid);
  if (it == items_.end())
    return;

  std::vector<int64_t> page_ids;
  page_ids.push_back(it->second->offline_id);

  model_->DeletePagesByOfflineId(
      page_ids, base::BindRepeating(&DownloadUIAdapter::OnDeletePagesDone,
                                    weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace offline_pages
