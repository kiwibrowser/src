// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/dom_storage_host.h"

#include "content/browser/dom_storage/dom_storage_area.h"
#include "content/browser/dom_storage/dom_storage_context_impl.h"
#include "content/browser/dom_storage/dom_storage_namespace.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

DOMStorageHost::DOMStorageHost(DOMStorageContextImpl* context)
    : context_(context) {
}

DOMStorageHost::~DOMStorageHost() {
  // Clear connections prior to releasing the context_.
  while (!connections_.empty())
    CloseStorageArea(connections_.begin()->first);
}

base::Optional<bad_message::BadMessageReason> DOMStorageHost::OpenStorageArea(
    int connection_id,
    const std::string& namespace_id,
    const url::Origin& origin) {
  if (HasConnection(connection_id))
    return bad_message::DSH_DUPLICATE_CONNECTION_ID;
  NamespaceAndArea references;
  references.namespace_ = context_->GetStorageNamespace(namespace_id);
  if (!references.namespace_.get())
    return context_->DiagnoseSessionNamespaceId(namespace_id);

  // namespace->OpenStorageArea() is called only once per process
  // (areas_open_count[area] is 0).
  references.area_ = references.namespace_->GetOpenStorageArea(origin);
  if (!references.area_ || !areas_open_count_[references.area_.get()]) {
    references.area_ = references.namespace_->OpenStorageArea(origin);
    DCHECK(references.area_.get());
    DCHECK_EQ(0, areas_open_count_[references.area_.get()]);
  }
  ++areas_open_count_[references.area_.get()];
  connections_[connection_id] = references;
  return base::nullopt;
}

void DOMStorageHost::CloseStorageArea(int connection_id) {
  const auto found = connections_.find(connection_id);
  if (found == connections_.end())
    return;
  DOMStorageArea* area = found->second.area_.get();
  DCHECK(areas_open_count_[area]);

  // namespace->CloseStorageArea() is called only once per process
  // (areas_open_count[area] becomes 0).
  if (--areas_open_count_[area] == 0) {
    found->second.namespace_->CloseStorageArea(area);
    areas_open_count_.erase(area);
  }
  connections_.erase(found);
}

bool DOMStorageHost::ExtractAreaValues(
    int connection_id, DOMStorageValuesMap* map) {
  map->clear();
  DOMStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return false;
  if (area->IsMapReloadNeeded()) {
    DOMStorageNamespace* ns = GetNamespace(connection_id);
    DCHECK(ns);
    context_->PurgeMemory(DOMStorageContextImpl::PURGE_IF_NEEDED);
  }
  area->ExtractValues(map);
  return true;
}

unsigned DOMStorageHost::GetAreaLength(int connection_id) {
  DOMStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return 0;
  return area->Length();
}

base::NullableString16 DOMStorageHost::GetAreaKey(int connection_id,
                                                  unsigned index) {
  DOMStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return base::NullableString16();
  return area->Key(index);
}

base::NullableString16 DOMStorageHost::GetAreaItem(int connection_id,
                                                   const base::string16& key) {
  DOMStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return base::NullableString16();
  return area->GetItem(key);
}

bool DOMStorageHost::SetAreaItem(int connection_id,
                                 const base::string16& key,
                                 const base::string16& value,
                                 const base::NullableString16& client_old_value,
                                 const GURL& page_url) {
  DOMStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return false;
  base::NullableString16 old_value;
  if (!area->SetItem(key, value, client_old_value, &old_value))
    return false;
  if (old_value.is_null() || old_value.string() != value)
    context_->NotifyItemSet(area, key, value, old_value, page_url);
  return true;
}

bool DOMStorageHost::RemoveAreaItem(
    int connection_id,
    const base::string16& key,
    const base::NullableString16& client_old_value,
    const GURL& page_url) {
  DOMStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return false;
  base::string16 old_value;
  if (!area->RemoveItem(key, client_old_value, &old_value))
    return false;
  context_->NotifyItemRemoved(area, key, old_value, page_url);
  return true;
}

bool DOMStorageHost::ClearArea(int connection_id, const GURL& page_url) {
  DOMStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return false;
  if (!area->Clear())
    return false;
  context_->NotifyAreaCleared(area, page_url);
  return true;
}

bool DOMStorageHost::HasAreaOpen(const std::string& namespace_id,
                                 const url::Origin& origin) const {
  for (const auto& it : connections_) {
    if (namespace_id == it.second.namespace_->namespace_id() &&
        origin == it.second.area_->origin()) {
      return true;
    }
  }
  return false;
}

DOMStorageArea* DOMStorageHost::GetOpenArea(int connection_id) const {
  const auto found = connections_.find(connection_id);
  if (found == connections_.end())
    return nullptr;
  return found->second.area_.get();
}

DOMStorageNamespace* DOMStorageHost::GetNamespace(int connection_id) const {
  const auto found = connections_.find(connection_id);
  if (found == connections_.end())
    return nullptr;
  return found->second.namespace_.get();
}

// NamespaceAndArea

DOMStorageHost::NamespaceAndArea::NamespaceAndArea() {}
DOMStorageHost::NamespaceAndArea::NamespaceAndArea(
    const NamespaceAndArea& other) = default;
DOMStorageHost::NamespaceAndArea::~NamespaceAndArea() {}

}  // namespace content
