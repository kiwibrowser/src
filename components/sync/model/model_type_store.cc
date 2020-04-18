// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/model_type_store.h"

#include <utility>

#include "components/sync/model_impl/in_memory_metadata_change_list.h"
#include "components/sync/model_impl/model_type_store_impl.h"

namespace syncer {

// static
void ModelTypeStore::CreateInMemoryStoreForTest(ModelType type,
                                                InitCallback callback) {
  ModelTypeStoreImpl::CreateInMemoryStoreForTest(type, std::move(callback));
}

// static
void ModelTypeStore::CreateStore(const std::string& path,
                                 ModelType type,
                                 InitCallback callback) {
  ModelTypeStoreImpl::CreateStore(type, path, std::move(callback));
}

ModelTypeStore::~ModelTypeStore() {}

// static
std::unique_ptr<MetadataChangeList>
ModelTypeStore::WriteBatch::CreateMetadataChangeList() {
  return std::make_unique<InMemoryMetadataChangeList>();
}

ModelTypeStore::WriteBatch::WriteBatch() {}

ModelTypeStore::WriteBatch::~WriteBatch() {}

void ModelTypeStore::WriteBatch::TakeMetadataChangesFrom(
    std::unique_ptr<MetadataChangeList> mcl) {
  static_cast<InMemoryMetadataChangeList*>(mcl.get())->TransferChangesTo(
      GetMetadataChangeList());
}

}  // namespace syncer
