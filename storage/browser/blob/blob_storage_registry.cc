// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_storage_registry.h"

#include <stddef.h>

#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "storage/browser/blob/blob_entry.h"
#include "storage/browser/blob/blob_url_utils.h"
#include "url/gurl.h"

namespace storage {

BlobStorageRegistry::BlobStorageRegistry() = default;

BlobStorageRegistry::~BlobStorageRegistry() {
  // Note: We don't bother calling the construction complete callbacks, as we
  // are only being destructed at the end of the life of the browser process.
  // So it shouldn't matter.
}

BlobEntry* BlobStorageRegistry::CreateEntry(
    const std::string& uuid,
    const std::string& content_type,
    const std::string& content_disposition) {
  DCHECK(blob_map_.find(uuid) == blob_map_.end());
  std::unique_ptr<BlobEntry> entry =
      std::make_unique<BlobEntry>(content_type, content_disposition);
  BlobEntry* entry_ptr = entry.get();
  blob_map_[uuid] = std::move(entry);
  return entry_ptr;
}

bool BlobStorageRegistry::DeleteEntry(const std::string& uuid) {
  return blob_map_.erase(uuid) == 1;
}

bool BlobStorageRegistry::HasEntry(const std::string& uuid) const {
  return blob_map_.find(uuid) != blob_map_.end();
}

BlobEntry* BlobStorageRegistry::GetEntry(const std::string& uuid) {
  auto found = blob_map_.find(uuid);
  if (found == blob_map_.end())
    return nullptr;
  return found->second.get();
}

const BlobEntry* BlobStorageRegistry::GetEntry(const std::string& uuid) const {
  return const_cast<BlobStorageRegistry*>(this)->GetEntry(uuid);
}

bool BlobStorageRegistry::CreateUrlMapping(const GURL& blob_url,
                                           const std::string& uuid) {
  DCHECK(!BlobUrlUtils::UrlHasFragment(blob_url));
  if (blob_map_.find(uuid) == blob_map_.end() || IsURLMapped(blob_url))
    return false;
  url_to_uuid_[blob_url] = uuid;
  return true;
}

bool BlobStorageRegistry::DeleteURLMapping(const GURL& blob_url,
                                           std::string* uuid) {
  DCHECK(!BlobUrlUtils::UrlHasFragment(blob_url));
  auto found = url_to_uuid_.find(blob_url);
  if (found == url_to_uuid_.end())
    return false;
  if (uuid)
    uuid->assign(found->second);
  url_to_uuid_.erase(found);
  return true;
}

bool BlobStorageRegistry::IsURLMapped(const GURL& blob_url) const {
  return url_to_uuid_.find(blob_url) != url_to_uuid_.end();
}

BlobEntry* BlobStorageRegistry::GetEntryFromURL(const GURL& url,
                                                std::string* uuid) {
  auto found = url_to_uuid_.find(BlobUrlUtils::ClearUrlFragment(url));
  if (found == url_to_uuid_.end())
    return nullptr;
  BlobEntry* entry = GetEntry(found->second);
  if (entry && uuid)
    uuid->assign(found->second);
  return entry;
}

void BlobStorageRegistry::AddTokenMapping(const base::UnguessableToken& token,
                                          const GURL& url,
                                          const std::string& uuid) {
  DCHECK(token_to_url_and_uuid_.find(token) == token_to_url_and_uuid_.end());
  token_to_url_and_uuid_.emplace(token, std::make_pair(url, uuid));
}

void BlobStorageRegistry::RemoveTokenMapping(
    const base::UnguessableToken& token) {
  DCHECK(token_to_url_and_uuid_.find(token) != token_to_url_and_uuid_.end());
  token_to_url_and_uuid_.erase(token);
}

bool BlobStorageRegistry::GetTokenMapping(const base::UnguessableToken& token,
                                          GURL* url,
                                          std::string* uuid) const {
  auto it = token_to_url_and_uuid_.find(token);
  if (it == token_to_url_and_uuid_.end())
    return false;
  *url = it->second.first;
  *uuid = it->second.second;
  return true;
}

}  // namespace storage
