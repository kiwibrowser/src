// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_CACHED_AREA_H_
#define CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_CACHED_AREA_H_

#include <map>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/nullable_string16.h"
#include "content/common/content_export.h"
#include "content/common/dom_storage/dom_storage_map.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "content/common/possibly_associated_interface_ptr.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/blink/public/platform/web_scoped_virtual_time_pauser.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace blink {
namespace scheduler {
class WebMainThreadScheduler;
}
}  // namespace blink

namespace content {
class LocalStorageArea;
class LocalStorageCachedAreas;

namespace mojom {
class StoragePartitionService;
class SessionStorageNamespace;
}

// An in-process implementation of LocalStorage using a LevelDB Mojo service.
// Maintains a complete cache of the origin's Map of key/value pairs for fast
// access. The cache is primed on first access and changes are written to the
// backend through the level db interface pointer. Mutations originating in
// other processes are applied to the cache via mojom::LevelDBObserver
// callbacks.
// There is one LocalStorageCachedArea for potentially many LocalStorageArea
// objects.
// TODO(dmurph): Rename to remove LocalStorage.
class CONTENT_EXPORT LocalStorageCachedArea
    : public mojom::LevelDBObserver,
      public base::RefCounted<LocalStorageCachedArea> {
 public:
  LocalStorageCachedArea(
      const std::string& namespace_id,
      const url::Origin& origin,
      mojom::SessionStorageNamespace* session_namespace,
      LocalStorageCachedAreas* cached_areas,
      blink::scheduler::WebMainThreadScheduler* main_thread_scheduler);
  LocalStorageCachedArea(
      const url::Origin& origin,
      mojom::StoragePartitionService* storage_partition_service,
      LocalStorageCachedAreas* cached_areas,
      blink::scheduler::WebMainThreadScheduler* main_thread_scheduler);

  // These correspond to blink::WebStorageArea.
  unsigned GetLength();
  base::NullableString16 GetKey(unsigned index);
  base::NullableString16 GetItem(const base::string16& key);
  bool SetItem(const base::string16& key,
               const base::string16& value,
               const GURL& page_url,
               const std::string& storage_area_id);
  void RemoveItem(const base::string16& key,
                  const GURL& page_url,
                  const std::string& storage_area_id);
  void Clear(const GURL& page_url, const std::string& storage_area_id);

  // Allow this object to keep track of the LocalStorageAreas corresponding to
  // it, which is needed for mutation event notifications.
  void AreaCreated(LocalStorageArea* area);
  void AreaDestroyed(LocalStorageArea* area);

  const std::string& namespace_id() { return namespace_id_; }
  const url::Origin& origin() { return origin_; }

  size_t memory_used() const { return map_ ? map_->memory_used() : 0; }

  bool IsSessionStorage() const { return !namespace_id_.empty(); }

 private:
  friend class base::RefCounted<LocalStorageCachedArea>;
  ~LocalStorageCachedArea() override;

  friend class LocalStorageCachedAreaTest;

  enum class FormatOption {
    kLocalStorageDetectFormat,
    kSessionStorageForceUTF16,
    kSessionStorageForceUTF8
  };

  static base::string16 Uint8VectorToString16(const std::vector<uint8_t>& input,
                                              FormatOption format_option);
  static std::vector<uint8_t> String16ToUint8Vector(const base::string16& input,
                                                    FormatOption format_option);

  // LevelDBObserver:
  void KeyAdded(const std::vector<uint8_t>& key,
                const std::vector<uint8_t>& value,
                const std::string& source) override;
  void KeyChanged(const std::vector<uint8_t>& key,
                  const std::vector<uint8_t>& new_value,
                  const std::vector<uint8_t>& old_value,
                  const std::string& source) override;
  void KeyDeleted(const std::vector<uint8_t>& key,
                  const std::vector<uint8_t>& old_value,
                  const std::string& source) override;
  void AllDeleted(const std::string& source) override;
  void ShouldSendOldValueOnMutations(bool value) override;

  // Common helper for KeyAdded() and KeyChanged()
  void KeyAddedOrChanged(const std::vector<uint8_t>& key,
                         const std::vector<uint8_t>& new_value,
                         const base::NullableString16& old_value,
                         const std::string& source);

  // Synchronously fetches the origin's local storage data if it hasn't been
  // fetched already.
  void EnsureLoaded();

  void OnSetItemComplete(const base::string16& key,
                         blink::WebScopedVirtualTimePauser virtual_time_pauser,
                         bool success);
  void OnRemoveItemComplete(
      const base::string16& key,
      blink::WebScopedVirtualTimePauser virtual_time_pauser,
      bool success);
  void OnClearComplete(blink::WebScopedVirtualTimePauser virtual_time_pauser,
                       bool success);
  void OnGetAllComplete(bool success);

  // Resets the object back to its newly constructed state.
  void Reset();

  std::string namespace_id_;
  url::Origin origin_;
  scoped_refptr<DOMStorageMap> map_;
  std::map<base::string16, int> ignore_key_mutations_;
  bool ignore_all_mutations_ = false;
  // See ShouldSendOldValueOnMutations().
  bool should_send_old_value_on_mutations_ = true;
  content::PossiblyAssociatedInterfacePtr<mojom::LevelDBWrapper> leveldb_;
  mojo::AssociatedBinding<mojom::LevelDBObserver> binding_;
  LocalStorageCachedAreas* cached_areas_;
  std::map<std::string, LocalStorageArea*> areas_;

  // Not owned.
  blink::scheduler::WebMainThreadScheduler* main_thread_scheduler_;

  base::WeakPtrFactory<LocalStorageCachedArea> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocalStorageCachedArea);
};

}  // namespace content

#endif  // CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_CACHED_AREA_H_
