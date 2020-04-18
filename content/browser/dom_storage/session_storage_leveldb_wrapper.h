// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_LEVELDB_WRAPPER_H_
#define CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_LEVELDB_WRAPPER_H_

#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "content/browser/dom_storage/session_storage_metadata.h"
#include "content/browser/leveldb_wrapper_impl.h"
#include "content/common/content_export.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "url/origin.h"

namespace content {
class SessionStorageDataMap;

// This class provides session storage access to the renderer by binding to the
// LevelDBWrapper mojom interface. It represents the data stored for a
// namespace-origin area.
//
// This class delegates calls to SessionStorageDataMap objects, and can share
// them with other SessionStorageLevelDBImpl instances to support shallow
// cloning (copy-on-write). This should be done through the |Clone()| method and
// not manually.
//
// During forking, this class is responsible for dealing with moving its
// observers from the SessionStorageDataMap's LevelDBWrapper to the new forked
// SessionStorageDataMap's LevelDBWrapper.
class CONTENT_EXPORT SessionStorageLevelDBWrapper
    : public mojom::LevelDBWrapper {
 public:
  using RegisterNewAreaMap =
      base::RepeatingCallback<scoped_refptr<SessionStorageMetadata::MapData>(
          SessionStorageMetadata::NamespaceEntry namespace_entry,
          const url::Origin& origin)>;

  // Creates a wrapper for the given |namespace_entry|-|origin| data area. All
  // LevelDBWrapper calls are delegated to the |data_map|. The
  // |register_new_map_callback| is called when a shared |data_map| needs to be
  // forked for the copy-on-write behavior and a new map needs to be registered.
  SessionStorageLevelDBWrapper(
      SessionStorageMetadata::NamespaceEntry namespace_entry,
      url::Origin origin,
      scoped_refptr<SessionStorageDataMap> data_map,
      RegisterNewAreaMap register_new_map_callback);
  ~SessionStorageLevelDBWrapper() override;

  // Creates a shallow copy clone for the new namespace entry.
  // This doesn't change the refcount of the underlying map - that operation
  // must be done using SessionStorageMetadata::RegisterShallowClonedNamespace.
  std::unique_ptr<SessionStorageLevelDBWrapper> Clone(
      SessionStorageMetadata::NamespaceEntry namespace_entry);

  void Bind(mojom::LevelDBWrapperAssociatedRequest request);

  bool IsBound() const { return binding_.is_bound(); }

  SessionStorageDataMap* data_map() { return shared_data_map_.get(); }

  // LevelDBWrapper:
  void AddObserver(mojom::LevelDBObserverAssociatedPtrInfo observer) override;
  void Put(const std::vector<uint8_t>& key,
           const std::vector<uint8_t>& value,
           const base::Optional<std::vector<uint8_t>>& client_old_value,
           const std::string& source,
           PutCallback callback) override;
  void Delete(const std::vector<uint8_t>& key,
              const base::Optional<std::vector<uint8_t>>& client_old_value,
              const std::string& source,
              DeleteCallback callback) override;
  void DeleteAll(const std::string& source,
                 DeleteAllCallback callback) override;
  void Get(const std::vector<uint8_t>& key, GetCallback callback) override;
  void GetAll(
      mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
      GetAllCallback callback) override;

 private:
  void OnConnectionError();

  enum class NewMapType { FORKED, EMPTY_FROM_DELETE_ALL };

  void CreateNewMap(NewMapType map_type,
                    const base::Optional<std::string>& delete_all_source);

  SessionStorageMetadata::NamespaceEntry namespace_entry_;
  url::Origin origin_;
  scoped_refptr<SessionStorageDataMap> shared_data_map_;
  RegisterNewAreaMap register_new_map_callback_;

  std::vector<mojo::InterfacePtrSetElementId> observer_ptrs_;
  mojo::AssociatedBinding<mojom::LevelDBWrapper> binding_;

  DISALLOW_COPY_AND_ASSIGN(SessionStorageLevelDBWrapper);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_LEVELDB_WRAPPER_H_
