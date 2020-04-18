// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/catalog/instance.h"

#include <memory>

#include "base/bind.h"
#include "base/values.h"
#include "services/catalog/entry.h"
#include "services/catalog/entry_cache.h"
#include "services/catalog/manifest_provider.h"

namespace catalog {
namespace {

void AddEntry(const Entry& entry, std::vector<mojom::EntryPtr>* ary) {
  mojom::EntryPtr entry_ptr(mojom::Entry::New());
  entry_ptr->name = entry.name();
  entry_ptr->display_name = entry.display_name();
  ary->push_back(std::move(entry_ptr));
}

}  // namespace

Instance::Instance(EntryCache* system_cache,
                   ManifestProvider* service_manifest_provider)
    : system_cache_(system_cache),
      service_manifest_provider_(service_manifest_provider) {}

Instance::~Instance() {}

void Instance::BindCatalog(mojom::CatalogRequest request) {
  catalog_bindings_.AddBinding(this, std::move(request));
}

const Entry* Instance::Resolve(const std::string& service_name) {
  DCHECK(system_cache_);
  const Entry* cached_entry = system_cache_->GetEntry(service_name);
  if (cached_entry)
    return cached_entry;

  std::unique_ptr<base::Value> new_manifest;
  if (service_manifest_provider_)
    new_manifest = service_manifest_provider_->GetManifest(service_name);

  if (!new_manifest) {
    LOG(ERROR) << "Unable to locate service manifest for " << service_name;
    return nullptr;
  }

  auto new_entry = Entry::Deserialize(*new_manifest);
  if (!new_entry) {
    LOG(ERROR) << "Malformed manifest for " << service_name;
    return nullptr;
  }

  cached_entry = const_cast<const Entry*>(new_entry.get());
  bool added = system_cache_->AddRootEntry(std::move(new_entry));
  DCHECK(added);
  return cached_entry;
}

void Instance::GetEntries(const base::Optional<std::vector<std::string>>& names,
                          GetEntriesCallback callback) {
  DCHECK(system_cache_);

  std::vector<mojom::EntryPtr> entries;
  if (!names.has_value()) {
    // TODO(beng): user catalog.
    for (const auto& entry : system_cache_->entries())
      AddEntry(*entry.second, &entries);
  } else {
    for (const std::string& name : names.value()) {
      const Entry* entry = system_cache_->GetEntry(name);
      // TODO(beng): user catalog.
      if (entry)
        AddEntry(*entry, &entries);
    }
  }
  std::move(callback).Run(std::move(entries));
}

void Instance::GetEntriesProvidingCapability(
    const std::string& capability,
    GetEntriesProvidingCapabilityCallback callback) {
  std::vector<mojom::EntryPtr> entries;
  for (const auto& entry : system_cache_->entries())
    if (entry.second->ProvidesCapability(capability))
      entries.push_back(mojom::Entry::From(*entry.second));
  std::move(callback).Run(std::move(entries));
}

}  // namespace catalog
