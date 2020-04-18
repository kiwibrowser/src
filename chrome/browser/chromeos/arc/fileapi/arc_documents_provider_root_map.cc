// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root_map.h"

#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root_map_factory.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/arc_service_manager.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace arc {

namespace {

struct DocumentsProviderSpec {
  const char* authority;
  const char* root_document_id;
};

// List of allowed documents providers for production.
constexpr DocumentsProviderSpec kDocumentsProviderWhitelist[] = {
    {"com.android.providers.media.documents", "images_root"},
    {"com.android.providers.media.documents", "videos_root"},
    {"com.android.providers.media.documents", "audio_root"},
};

}  // namespace

// static
ArcDocumentsProviderRootMap* ArcDocumentsProviderRootMap::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcDocumentsProviderRootMapFactory::GetForBrowserContext(context);
}

// static
ArcDocumentsProviderRootMap*
ArcDocumentsProviderRootMap::GetForArcBrowserContext() {
  return GetForBrowserContext(ArcServiceManager::Get()->browser_context());
}

ArcDocumentsProviderRootMap::ArcDocumentsProviderRootMap(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ArcFileSystemOperationRunner* runner =
      ArcFileSystemOperationRunner::GetForBrowserContext(profile);
  // ArcDocumentsProviderRootMap is created only for the profile with ARC
  // in ArcDocumentsProviderRootMapFactory.
  DCHECK(runner);

  for (const auto& spec : kDocumentsProviderWhitelist) {
    map_[Key(spec.authority, spec.root_document_id)] =
        std::make_unique<ArcDocumentsProviderRoot>(runner, spec.authority,
                                                   spec.root_document_id);
  }
}

ArcDocumentsProviderRootMap::~ArcDocumentsProviderRootMap() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(map_.empty());
}

ArcDocumentsProviderRoot* ArcDocumentsProviderRootMap::ParseAndLookup(
    const storage::FileSystemURL& url,
    base::FilePath* path) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::string authority;
  std::string root_document_id;
  base::FilePath tmp_path;
  if (!ParseDocumentsProviderUrl(url, &authority, &root_document_id, &tmp_path))
    return nullptr;

  ArcDocumentsProviderRoot* root = Lookup(authority, root_document_id);
  if (!root)
    return nullptr;

  *path = tmp_path;
  return root;
}

ArcDocumentsProviderRoot* ArcDocumentsProviderRootMap::Lookup(
    const std::string& authority,
    const std::string& root_document_id) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto iter = map_.find(Key(authority, root_document_id));
  if (iter == map_.end())
    return nullptr;
  return iter->second.get();
}

void ArcDocumentsProviderRootMap::Shutdown() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // ArcDocumentsProviderRoot has a reference to another KeyedService
  // (ArcFileSystemOperationRunner), so we need to destruct them on shutdown.
  map_.clear();
}

}  // namespace arc
