// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/service_transfer_cache.h"

#include "base/sys_info.h"

namespace gpu {
namespace {
size_t CacheSizeLimit() {
  if (base::SysInfo::IsLowEndDevice()) {
    return 4 * 1024 * 1024;
  } else {
    return 128 * 1024 * 1024;
  }
}

}  // namespace

ServiceTransferCache::CacheEntryInternal::CacheEntryInternal(
    base::Optional<ServiceDiscardableHandle> handle,
    std::unique_ptr<cc::ServiceTransferCacheEntry> entry)
    : handle(handle), entry(std::move(entry)) {}

ServiceTransferCache::CacheEntryInternal::~CacheEntryInternal() = default;

ServiceTransferCache::CacheEntryInternal::CacheEntryInternal(
    CacheEntryInternal&& other) = default;

ServiceTransferCache::CacheEntryInternal&
ServiceTransferCache::CacheEntryInternal::operator=(
    CacheEntryInternal&& other) = default;

ServiceTransferCache::ServiceTransferCache()
    : entries_(EntryCache::NO_AUTO_EVICT),
      cache_size_limit_(CacheSizeLimit()) {}

ServiceTransferCache::~ServiceTransferCache() = default;

bool ServiceTransferCache::CreateLockedEntry(
    cc::TransferCacheEntryType entry_type,
    uint32_t entry_id,
    ServiceDiscardableHandle handle,
    GrContext* context,
    base::span<uint8_t> data) {
  auto key = std::make_pair(entry_type, entry_id);
  auto found = entries_.Peek(key);
  if (found != entries_.end())
    return false;

  std::unique_ptr<cc::ServiceTransferCacheEntry> entry =
      cc::ServiceTransferCacheEntry::Create(entry_type);
  if (!entry)
    return false;

  if (!entry->Deserialize(context, data))
    return false;

  total_size_ += entry->CachedSize();
  entries_.Put(key, CacheEntryInternal(handle, std::move(entry)));
  EnforceLimits();
  return true;
}

void ServiceTransferCache::CreateLocalEntry(
    uint32_t entry_id,
    std::unique_ptr<cc::ServiceTransferCacheEntry> entry) {
  if (!entry)
    return;

  DeleteEntry(entry->Type(), entry_id);

  total_size_ += entry->CachedSize();

  auto key = std::make_pair(entry->Type(), entry_id);
  entries_.Put(key, CacheEntryInternal(base::nullopt, std::move(entry)));
  EnforceLimits();
}

bool ServiceTransferCache::UnlockEntry(cc::TransferCacheEntryType entry_type,
                                       uint32_t entry_id) {
  auto key = std::make_pair(entry_type, entry_id);
  auto found = entries_.Peek(key);
  if (found == entries_.end())
    return false;

  if (!found->second.handle)
    return false;
  found->second.handle->Unlock();
  return true;
}

bool ServiceTransferCache::DeleteEntry(cc::TransferCacheEntryType entry_type,
                                       uint32_t entry_id) {
  auto key = std::make_pair(entry_type, entry_id);
  auto found = entries_.Peek(key);
  if (found == entries_.end())
    return false;

  if (found->second.handle)
    found->second.handle->ForceDelete();
  total_size_ -= found->second.entry->CachedSize();
  entries_.Erase(found);
  return true;
}

cc::ServiceTransferCacheEntry* ServiceTransferCache::GetEntry(
    cc::TransferCacheEntryType entry_type,
    uint32_t entry_id) {
  auto key = std::make_pair(entry_type, entry_id);
  auto found = entries_.Get(key);
  if (found == entries_.end())
    return nullptr;
  return found->second.entry.get();
}

void ServiceTransferCache::EnforceLimits() {
  for (auto it = entries_.rbegin(); it != entries_.rend();) {
    if (total_size_ <= cache_size_limit_) {
      return;
    }
    if (it->second.handle && !it->second.handle->Delete()) {
      ++it;
      continue;
    }

    total_size_ -= it->second.entry->CachedSize();
    it = entries_.Erase(it);
  }
}

}  // namespace gpu
