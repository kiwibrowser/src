// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_DOM_STORAGE_DOM_STORAGE_MAP_H_
#define CONTENT_COMMON_DOM_STORAGE_DOM_STORAGE_MAP_H_

#include <stddef.h>

#include <map>

#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "content/common/dom_storage/dom_storage_types.h"

namespace content {

// A wrapper around a std::map that adds refcounting and
// tracks the size in bytes of the keys/values, enforcing a quota.
// See class comments for DOMStorageContextImpl for a larger overview.
class CONTENT_EXPORT DOMStorageMap
    : public base::RefCountedThreadSafe<DOMStorageMap> {
 public:
  DOMStorageMap(size_t quota, bool only_keys);
  explicit DOMStorageMap(size_t quota);

  unsigned Length() const;
  base::NullableString16 Key(unsigned index);

  // Sets and removes items from the storage. Old value is wriiten into
  // |old_value| if it's not null and |has_only_keys| is false. |old_value| is
  // not accessed or modified when |has_only_keys|.
  bool SetItem(const base::string16& key, const base::string16& value,
               base::NullableString16* old_value);
  bool RemoveItem(const base::string16& key, base::string16* old_value);

  // Returns value for the given |key|. Use only when |has_only_keys| is false.
  base::NullableString16 GetItem(const base::string16& key) const;

  // Writes a copy of the current set of map_ to the |map|. Use only when
  // |has_only_keys| is false.
  void ExtractValues(DOMStorageValuesMap* map) const;

  // Swaps the values from |map| to |keys_values_| in this instances,
  // Note: to grandfather in pre-existing files that are overbudget,
  // this method does not do quota checking. Use only when |has_only_keys| is
  // false.
  void SwapValues(DOMStorageValuesMap* map);

  // Stores the keys and sizes of values from |map| to |keys_only_|. Use only
  // when |has_only_keys| is true. This method does not do quota checking.
  void TakeKeysFrom(const DOMStorageValuesMap& map);

  // Creates a new instance of DOMStorageMap containing
  // a deep copy of the map.
  DOMStorageMap* DeepCopy() const;

  const DOMStorageValuesMap& keys_values() const { return keys_values_; }
  size_t storage_used() const { return storage_used_; }
  size_t memory_used() const { return memory_used_; }
  size_t quota() const { return quota_; }
  void set_quota(size_t quota) { quota_ = quota; }
  bool has_only_keys() const { return has_only_keys_; }

  static size_t CountBytes(const DOMStorageValuesMap& values);

 private:
  friend class base::RefCountedThreadSafe<DOMStorageMap>;
  FRIEND_TEST_ALL_PREFIXES(DOMStorageMapParamTest, EnforcesQuota);

  using KeysMap = std::map<base::string16, size_t>;

  ~DOMStorageMap();

  template <typename MapType>
  bool SetItemInternal(MapType* map_type,
                       const base::string16& key,
                       const typename MapType::mapped_type& value,
                       typename MapType::mapped_type* old_value);
  template <typename MapType>
  bool RemoveItemInternal(MapType* map_type,
                          const base::string16& key,
                          typename MapType::mapped_type* old_value);

  void ResetKeyIterator();

  // Used when |has_only_keys_| is false.
  DOMStorageValuesMap keys_values_;
  // Used when |has_only_keys_| is true.
  KeysMap keys_only_;

  DOMStorageValuesMap::const_iterator keys_values_iterator_;
  KeysMap::const_iterator keys_only_iterator_;
  unsigned last_key_index_;
  size_t storage_used_;
  size_t memory_used_;
  size_t quota_;
  const bool has_only_keys_;
};

}  // namespace content

#endif  // CONTENT_COMMON_DOM_STORAGE_DOM_STORAGE_MAP_H_
