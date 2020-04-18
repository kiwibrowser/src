// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_DATABASE_ADAPTER_H_
#define CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_DATABASE_ADAPTER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/dom_storage/dom_storage_database_adapter.h"
#include "url/origin.h"

namespace content {

class SessionStorageDatabase;

class SessionStorageDatabaseAdapter : public DOMStorageDatabaseAdapter {
 public:
  SessionStorageDatabaseAdapter(
      SessionStorageDatabase* db,
      const std::string& permanent_namespace_id,
      const std::vector<std::string>& original_permanent_namespace_ids,
      const url::Origin& origin);
  ~SessionStorageDatabaseAdapter() override;
  void ReadAllValues(DOMStorageValuesMap* result) override;
  bool CommitChanges(bool clear_all_first,
                     const DOMStorageValuesMap& changes) override;

 private:
  scoped_refptr<SessionStorageDatabase> db_;
  std::string permanent_namespace_id_;
  // IDs of original databases in order of ShallowCopy(s).
  std::vector<std::string> original_permanent_namespace_ids_;
  url::Origin origin_;

  DISALLOW_COPY_AND_ASSIGN(SessionStorageDatabaseAdapter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_DATABASE_ADAPTER_H_
