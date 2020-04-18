// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/invalid_directory_backing_store.h"

namespace syncer {
namespace syncable {

InvalidDirectoryBackingStore::InvalidDirectoryBackingStore()
    : DirectoryBackingStore("some_fake_user") {}

InvalidDirectoryBackingStore::~InvalidDirectoryBackingStore() {}

DirOpenResult InvalidDirectoryBackingStore::Load(
    Directory::MetahandlesMap* handles_map,
    JournalIndex* delete_journals,
    MetahandleSet* metahandles_to_purge,
    Directory::KernelLoadInfo* kernel_load_info) {
  return FAILED_OPEN_DATABASE;
}

}  // namespace syncable
}  // namespace syncer
