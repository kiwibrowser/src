// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_CLOUD_RESOURCE_CACHE_H_
#define COMPONENTS_POLICY_CORE_COMMON_CLOUD_RESOURCE_CACHE_H_

#include <map>
#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/policy/policy_export.h"

namespace base {
class SequencedTaskRunner;
}

namespace policy {

// Manages storage of data at a given path. The data is keyed by a key and
// a subkey, and can be queried by (key, subkey) or (key) lookups.
// The contents of the cache have to be manually cleared using Delete() or
// Purge*().
// The class can be instantiated on any thread but from then on, it must be
// accessed via the |task_runner| only. The |task_runner| must support file I/O.
class POLICY_EXPORT ResourceCache {
 public:
  explicit ResourceCache(const base::FilePath& cache_path,
                         scoped_refptr<base::SequencedTaskRunner> task_runner);
  virtual ~ResourceCache();

  // Stores |data| under (key, subkey). Returns true if the store suceeded, and
  // false otherwise.
  bool Store(const std::string& key,
             const std::string& subkey,
             const std::string& data);

  // Loads the contents of (key, subkey) into |data| and returns true. Returns
  // false if (key, subkey) isn't found or if there is a problem reading the
  // data.
  bool Load(const std::string& key,
            const std::string& subkey,
            std::string* data);

  // Loads all the subkeys of |key| into |contents|.
  void LoadAllSubkeys(const std::string& key,
                      std::map<std::string, std::string>* contents);

  // Deletes (key, subkey).
  void Delete(const std::string& key, const std::string& subkey);

  // Deletes all the subkeys of |key|.
  void Clear(const std::string& key);

  // Deletes the subkeys of |key| for which the |filter| returns true.
  typedef base::Callback<bool(const std::string&)> SubkeyFilter;
  void FilterSubkeys(const std::string& key, const SubkeyFilter& filter);

  // Deletes all keys not in |keys_to_keep|, along with their subkeys.
  void PurgeOtherKeys(const std::set<std::string>& keys_to_keep);

  // Deletes all the subkeys of |key| not in |subkeys_to_keep|.
  void PurgeOtherSubkeys(const std::string& key,
                         const std::set<std::string>& subkeys_to_keep);

 private:
  // Points |path| at the cache directory for |key| and returns whether the
  // directory exists. If |allow_create| is |true|, the directory is created if
  // it did not exist yet.
  bool VerifyKeyPath(const std::string& key,
                     bool allow_create,
                     base::FilePath* path);

  // Points |subkey_path| at the file in which data for (key, subkey) should be
  // stored and returns whether the parent directory of this file exists. If
  // |allow_create_key| is |true|, the directory is created if it did not exist
  // yet. This method does not check whether the file at |subkey_path| exists or
  // not.
  bool VerifyKeyPathAndGetSubkeyPath(const std::string& key,
                                     bool allow_create_key,
                                     const std::string& subkey,
                                     base::FilePath* subkey_path);

  base::FilePath cache_dir_;

  // Task runner that |this| runs on.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ResourceCache);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_CLOUD_RESOURCE_CACHE_H_
