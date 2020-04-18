// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "components/offline_items_collection/core/offline_item.h"

namespace offline_items_collection {

namespace {

template <typename T, typename U>
bool MapContainsValue(const std::map<T, U>& map, U value) {
  for (const auto& it : map) {
    if (it.second == value)
      return true;
  }
  return false;
}

}  // namespace

OfflineContentAggregator::OfflineContentAggregator()
    : weak_ptr_factory_(this) {}

OfflineContentAggregator::~OfflineContentAggregator() = default;

void OfflineContentAggregator::RegisterProvider(
    const std::string& name_space,
    OfflineContentProvider* provider) {
  // Validate that this is the first OfflineContentProvider registered that is
  // associated with |name_space|.
  DCHECK(providers_.find(name_space) == providers_.end());

  // Only set up the connection to the provider if the provider isn't associated
  // with any other namespace.
  if (!MapContainsValue(providers_, provider))
    provider->AddObserver(this);

  providers_[name_space] = provider;
}

void OfflineContentAggregator::UnregisterProvider(
    const std::string& name_space) {
  auto provider_it = providers_.find(name_space);

  OfflineContentProvider* provider = provider_it->second;
  providers_.erase(provider_it);
  pending_providers_.erase(provider);

  // Only clean up the connection to the provider if the provider isn't
  // associated with any other namespace.
  if (!MapContainsValue(providers_, provider)) {
    provider->RemoveObserver(this);
  }
}

void OfflineContentAggregator::OpenItem(const ContentId& id) {
  auto it = providers_.find(id.name_space);

  if (it == providers_.end())
    return;

  it->second->OpenItem(id);
}

void OfflineContentAggregator::RemoveItem(const ContentId& id) {
  auto it = providers_.find(id.name_space);

  if (it == providers_.end())
    return;

  it->second->RemoveItem(id);
}

void OfflineContentAggregator::CancelDownload(const ContentId& id) {
  auto it = providers_.find(id.name_space);

  if (it == providers_.end())
    return;

  it->second->CancelDownload(id);
}

void OfflineContentAggregator::PauseDownload(const ContentId& id) {
  auto it = providers_.find(id.name_space);

  if (it == providers_.end())
    return;

  it->second->PauseDownload(id);
}

void OfflineContentAggregator::ResumeDownload(const ContentId& id,
                                              bool has_user_gesture) {
  auto it = providers_.find(id.name_space);

  if (it == providers_.end())
    return;

  it->second->ResumeDownload(id, has_user_gesture);
}

void OfflineContentAggregator::GetItemById(const ContentId& id,
                                           SingleItemCallback callback) {
  auto it = providers_.find(id.name_space);
  if (it == providers_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), base::nullopt));
    return;
  }

  it->second->GetItemById(
      id, base::BindOnce(&OfflineContentAggregator::OnGetItemByIdDone,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void OfflineContentAggregator::OnGetItemByIdDone(
    SingleItemCallback callback,
    const base::Optional<OfflineItem>& item) {
  std::move(callback).Run(item);
}

void OfflineContentAggregator::GetAllItems(MultipleItemCallback callback) {
  // If there is already a call in progress, queue up the callback and wait for
  // the results.
  if (!multiple_item_get_callbacks_.empty()) {
    multiple_item_get_callbacks_.push_back(std::move(callback));
    return;
  }

  DCHECK(aggregated_items_.empty());
  for (auto provider_it : providers_) {
    auto* provider = provider_it.second;

    provider->GetAllItems(
        base::BindOnce(&OfflineContentAggregator::OnGetAllItemsDone,
                       weak_ptr_factory_.GetWeakPtr(), provider));
    pending_providers_.insert(provider);
  }

  if (pending_providers_.empty()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), OfflineItemList()));
    return;
  }

  multiple_item_get_callbacks_.push_back(std::move(callback));
}

void OfflineContentAggregator::OnGetAllItemsDone(
    OfflineContentProvider* provider,
    const OfflineItemList& items) {
  aggregated_items_.insert(aggregated_items_.end(), items.begin(), items.end());
  pending_providers_.erase(provider);
  if (!pending_providers_.empty()) {
    return;
  }

  auto item_vec = std::move(aggregated_items_);
  auto callbacks = std::move(multiple_item_get_callbacks_);

  for (auto& callback : callbacks)
    std::move(callback).Run(item_vec);
}

void OfflineContentAggregator::GetVisualsForItem(
    const ContentId& id,
    const VisualsCallback& callback) {
  auto it = providers_.find(id.name_space);

  if (it == providers_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback, id, nullptr));
    return;
  }

  it->second->GetVisualsForItem(id, callback);
}

void OfflineContentAggregator::AddObserver(
    OfflineContentProvider::Observer* observer) {
  DCHECK(observer);
  if (observers_.HasObserver(observer))
    return;

  observers_.AddObserver(observer);
}

void OfflineContentAggregator::RemoveObserver(
    OfflineContentProvider::Observer* observer) {
  DCHECK(observer);
  if (!observers_.HasObserver(observer))
    return;

  observers_.RemoveObserver(observer);
}

void OfflineContentAggregator::OnItemsAdded(const OfflineItemList& items) {
  for (auto& observer : observers_)
    observer.OnItemsAdded(items);
}

void OfflineContentAggregator::OnItemRemoved(const ContentId& id) {
  for (auto& observer : observers_)
    observer.OnItemRemoved(id);
}

void OfflineContentAggregator::OnItemUpdated(const OfflineItem& item) {
  for (auto& observer : observers_)
    observer.OnItemUpdated(item);
}

}  // namespace offline_items_collection
