// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/session_storage_leveldb_wrapper.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "content/browser/dom_storage/session_storage_data_map.h"
#include "content/common/dom_storage/dom_storage_types.h"
#include "third_party/leveldatabase/env_chromium.h"

namespace content {

SessionStorageLevelDBWrapper::SessionStorageLevelDBWrapper(
    SessionStorageMetadata::NamespaceEntry namespace_entry,
    url::Origin origin,
    scoped_refptr<SessionStorageDataMap> data_map,
    RegisterNewAreaMap register_new_map_callback)
    : namespace_entry_(namespace_entry),
      origin_(std::move(origin)),
      shared_data_map_(std::move(data_map)),
      register_new_map_callback_(std::move(register_new_map_callback)),
      binding_(this) {}

SessionStorageLevelDBWrapper::~SessionStorageLevelDBWrapper() {
  if (binding_.is_bound())
    shared_data_map_->RemoveBindingReference();
}

void SessionStorageLevelDBWrapper::Bind(
    mojom::LevelDBWrapperAssociatedRequest request) {
  DCHECK(!IsBound());
  shared_data_map_->AddBindingReference();
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(
      base::BindOnce(&SessionStorageLevelDBWrapper::OnConnectionError,
                     base::Unretained(this)));
}

std::unique_ptr<SessionStorageLevelDBWrapper>
SessionStorageLevelDBWrapper::Clone(
    SessionStorageMetadata::NamespaceEntry namespace_entry) {
  DCHECK(namespace_entry_ != namespace_entry);
  return base::WrapUnique(new SessionStorageLevelDBWrapper(
      namespace_entry, origin_, shared_data_map_, register_new_map_callback_));
}

// LevelDBWrapper:
void SessionStorageLevelDBWrapper::AddObserver(
    mojom::LevelDBObserverAssociatedPtrInfo observer) {
  mojom::LevelDBObserverAssociatedPtr observer_ptr;
  observer_ptr.Bind(std::move(observer));
  mojo::InterfacePtrSetElementId ptr_id =
      shared_data_map_->level_db_wrapper()->AddObserver(
          std::move(observer_ptr));
  observer_ptrs_.push_back(ptr_id);
}

void SessionStorageLevelDBWrapper::Put(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& value,
    const base::Optional<std::vector<uint8_t>>& client_old_value,
    const std::string& source,
    PutCallback callback) {
  DCHECK(IsBound());
  DCHECK_NE(0, shared_data_map_->map_data()->ReferenceCount());
  if (shared_data_map_->map_data()->ReferenceCount() > 1)
    CreateNewMap(NewMapType::FORKED, base::nullopt);
  shared_data_map_->level_db_wrapper()->Put(key, value, client_old_value,
                                            source, std::move(callback));
}

void SessionStorageLevelDBWrapper::Delete(
    const std::vector<uint8_t>& key,
    const base::Optional<std::vector<uint8_t>>& client_old_value,
    const std::string& source,
    DeleteCallback callback) {
  DCHECK(IsBound());
  DCHECK_NE(0, shared_data_map_->map_data()->ReferenceCount());
  if (shared_data_map_->map_data()->ReferenceCount() > 1)
    CreateNewMap(NewMapType::FORKED, base::nullopt);
  shared_data_map_->level_db_wrapper()->Delete(key, client_old_value, source,
                                               std::move(callback));
}

void SessionStorageLevelDBWrapper::DeleteAll(const std::string& source,
                                             DeleteAllCallback callback) {
  DCHECK(IsBound());
  DCHECK_NE(0, shared_data_map_->map_data()->ReferenceCount());
  if (shared_data_map_->map_data()->ReferenceCount() > 1) {
    CreateNewMap(NewMapType::EMPTY_FROM_DELETE_ALL, source);
    std::move(callback).Run(true);
    return;
  }
  shared_data_map_->level_db_wrapper()->DeleteAll(source, std::move(callback));
}

void SessionStorageLevelDBWrapper::Get(const std::vector<uint8_t>& key,
                                       GetCallback callback) {
  DCHECK(IsBound());
  DCHECK_NE(0, shared_data_map_->map_data()->ReferenceCount());
  shared_data_map_->level_db_wrapper()->Get(key, std::move(callback));
}

void SessionStorageLevelDBWrapper::GetAll(
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
    GetAllCallback callback) {
  DCHECK(IsBound());
  DCHECK_NE(0, shared_data_map_->map_data()->ReferenceCount());
  shared_data_map_->level_db_wrapper()->GetAll(std::move(complete_callback),
                                               std::move(callback));
}

// Note: this can be called after invalidation of the |namespace_entry_|.
void SessionStorageLevelDBWrapper::OnConnectionError() {
  shared_data_map_->RemoveBindingReference();
  // Make sure we totally unbind the binding - this doesn't seem to happen
  // automatically on connection error. The bound status is used in the
  // destructor to know if |RemoveBindingReference| was already called.
  if (binding_.is_bound())
    binding_.Unbind();
}

void SessionStorageLevelDBWrapper::CreateNewMap(
    NewMapType map_type,
    const base::Optional<std::string>& delete_all_source) {
  std::vector<mojom::LevelDBObserverAssociatedPtr> ptrs_to_move;
  for (const mojo::InterfacePtrSetElementId& ptr_id : observer_ptrs_) {
    DCHECK(shared_data_map_->level_db_wrapper()->HasObserver(ptr_id));
    ptrs_to_move.push_back(
        shared_data_map_->level_db_wrapper()->RemoveObserver(ptr_id));
  }
  observer_ptrs_.clear();
  shared_data_map_->RemoveBindingReference();
  switch (map_type) {
    case NewMapType::FORKED:
      shared_data_map_ = SessionStorageDataMap::CreateClone(
          shared_data_map_->listener(),
          register_new_map_callback_.Run(namespace_entry_, origin_),
          shared_data_map_->level_db_wrapper());
      break;
    case NewMapType::EMPTY_FROM_DELETE_ALL: {
      // The code optimizes the 'delete all' for shared maps by just creating
      // a new map instead of forking. However, we still need the observers to
      // be correctly called. To do that, we manually call them here.
      shared_data_map_ = SessionStorageDataMap::Create(
          shared_data_map_->listener(),
          register_new_map_callback_.Run(namespace_entry_, origin_),
          shared_data_map_->level_db_wrapper()->database());
      for (auto& ptr : ptrs_to_move) {
        ptr->AllDeleted(delete_all_source.value_or("\n"));
      }
      break;
    }
  }
  shared_data_map_->AddBindingReference();

  for (auto& observer_ptr : ptrs_to_move) {
    observer_ptrs_.push_back(shared_data_map_->level_db_wrapper()->AddObserver(
        std::move(observer_ptr)));
  }
}

}  // namespace content
