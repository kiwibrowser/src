// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_DATA_MAP_H_
#define CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_DATA_MAP_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "content/browser/dom_storage/session_storage_metadata.h"
#include "content/browser/leveldb_wrapper_impl.h"
#include "content/common/content_export.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace content {

// Holds the LevelDBWrapper for a session storage data map. Every
// namespace-origin area has a data map. To support shallow copying of the data
// (copy-on-write), a single data map can be shared between multiple namespaces.
// Thus this class is refcounted. This class has a one-to-one relationship with
// the SessionStorageMetadata::MapData object, accessible from |map_data()|.
//
// Neither this data map nor the inner LevelDBWrapper is bound to, as it needs
// to be shared between multiple connections if it is shallow-copied. However,
// it does allow it's user to keep track of the number of binding using
// |binding_count()|, |AddBindingReference()|, and |RemoveBindingReference()|.
class CONTENT_EXPORT SessionStorageDataMap final
    : public LevelDBWrapperImpl::Delegate,
      public base::RefCounted<SessionStorageDataMap> {
 public:
  class CONTENT_EXPORT Listener {
   public:
    virtual ~Listener() {}
    virtual void OnDataMapCreation(const std::vector<uint8_t>& map_id,
                                   SessionStorageDataMap* map) = 0;
    virtual void OnDataMapDestruction(const std::vector<uint8_t>& map_id) = 0;
    virtual void OnCommitResult(leveldb::mojom::DatabaseError error) = 0;
  };

  static scoped_refptr<SessionStorageDataMap> Create(
      Listener* listener,
      scoped_refptr<SessionStorageMetadata::MapData> map_data,
      leveldb::mojom::LevelDBDatabase* database);

  static scoped_refptr<SessionStorageDataMap> CreateClone(
      Listener* listener,
      scoped_refptr<SessionStorageMetadata::MapData> map_data,
      LevelDBWrapperImpl* clone_from);

  Listener* listener() const { return listener_; }

  LevelDBWrapperImpl* level_db_wrapper() { return level_db_wrapper_ptr_; }

  scoped_refptr<SessionStorageMetadata::MapData> map_data() {
    return map_data_.get();
  }

  int binding_count() { return binding_count_; }
  void AddBindingReference() { ++binding_count_; }
  // When the binding count reaches 0, we schedule an immediate commit on our
  // wrapper, but we don't close the connection.
  void RemoveBindingReference();

  // Note: this is irrelevant, as the parent wrapper is handling binding.
  void OnNoBindings() override {}

  std::vector<leveldb::mojom::BatchedOperationPtr> PrepareToCommit() override;

  void DidCommit(leveldb::mojom::DatabaseError error) override;

 private:
  friend class base::RefCounted<SessionStorageDataMap>;

  SessionStorageDataMap(
      Listener* listener,
      scoped_refptr<SessionStorageMetadata::MapData> map_entry,
      leveldb::mojom::LevelDBDatabase* database);
  SessionStorageDataMap(
      Listener* listener,
      scoped_refptr<SessionStorageMetadata::MapData> map_entry,
      LevelDBWrapperImpl* forking_from);
  ~SessionStorageDataMap() override;

  static LevelDBWrapperImpl::Options GetOptions();

  Listener* listener_;
  int binding_count_ = 0;
  scoped_refptr<SessionStorageMetadata::MapData> map_data_;
  std::unique_ptr<LevelDBWrapperImpl> wrapper_impl_;
  // Holds the same value as |wrapper_impl_|. The reason for this is that
  // during destruction of the LevelDBWrapperImpl instance we might still get
  // called and need access  to the LevelDBWrapperImpl instance. The
  // unique_ptr could already be null, but this field should still be valid.
  // TODO(dmurph): Change delegate ownership so this doesn't have to be done.
  LevelDBWrapperImpl* level_db_wrapper_ptr_;

  DISALLOW_COPY_AND_ASSIGN(SessionStorageDataMap);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_DATA_MAP_H_
