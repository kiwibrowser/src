// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/test/test_directory_backing_store.h"

#include "base/logging.h"

namespace syncer {
namespace syncable {

TestDirectoryBackingStore::TestDirectoryBackingStore(
    const std::string& dir_name,
    sql::Connection* db)
    : DirectoryBackingStore(dir_name, db) {}

TestDirectoryBackingStore::~TestDirectoryBackingStore() {
  // This variant of the DirectoryBackingStore does not own its connection, so
  // we take care to not delete it here.
  ignore_result(db_.release());
}

DirOpenResult TestDirectoryBackingStore::Load(
    Directory::MetahandlesMap* handles_map,
    JournalIndex* delete_journals,
    MetahandleSet* metahandles_to_purge,
    Directory::KernelLoadInfo* kernel_load_info) {
  DCHECK(db_->is_open());

  if (!InitializeTables())
    return FAILED_OPEN_DATABASE;

  if (!LoadEntries(handles_map, metahandles_to_purge))
    return FAILED_DATABASE_CORRUPT;
  if (!LoadDeleteJournals(delete_journals))
    return FAILED_DATABASE_CORRUPT;
  if (!LoadInfo(kernel_load_info))
    return FAILED_DATABASE_CORRUPT;
  if (!VerifyReferenceIntegrity(handles_map))
    return FAILED_DATABASE_CORRUPT;

  return OPENED;
}

bool TestDirectoryBackingStore::DeleteEntries(const MetahandleSet& handles) {
  return DirectoryBackingStore::DeleteEntries(
      DirectoryBackingStore::METAS_TABLE, handles);
}

}  // namespace syncable
}  // namespace syncer
