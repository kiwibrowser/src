// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/catalog/entry_cache.h"

#include "services/catalog/entry.h"

namespace catalog {

EntryCache::EntryCache() {}

EntryCache::~EntryCache() {}

bool EntryCache::AddRootEntry(std::unique_ptr<Entry> entry) {
  DCHECK(entry);
  const std::string& name = entry->name();
  if (!AddEntry(entry.get()))
    return false;
  root_entries_.insert(std::make_pair(name, std::move(entry)));
  return true;
}

const Entry* EntryCache::GetEntry(const std::string& name) {
  auto iter = entries_.find(name);
  if (iter == entries_.end())
    return nullptr;
  return iter->second;
}

bool EntryCache::AddEntry(const Entry* entry) {
  auto root_iter = root_entries_.find(entry->name());
  if (root_iter != root_entries_.end()) {
    RemoveEntry(root_iter->second.get());
    root_entries_.erase(root_iter);
  } else {
    auto entry_iter = entries_.find(entry->name());
    if (entry_iter != entries_.end()) {
      // There's already a non-root entry for this name, so we change nothing.
      return false;
    }
  }

  entries_.insert({ entry->name(), entry });
  for (const auto& child : entry->children())
    AddEntry(child.get());
  return true;
}

void EntryCache::RemoveEntry(const Entry* entry) {
  auto iter = entries_.find(entry->name());
  if (iter->second == entry)
    entries_.erase(iter);
  for (const auto& child : entry->children())
    RemoveEntry(child.get());
}

}  // namespace catalog
