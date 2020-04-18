// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NET_QUOTA_POLICY_COOKIE_STORE_H_
#define CONTENT_BROWSER_NET_QUOTA_POLICY_COOKIE_STORE_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "net/cookies/cookie_monster.h"
#include "net/extras/sqlite/sqlite_persistent_cookie_store.h"

namespace net {
class CanonicalCookie;
}  // namespace net

namespace storage {
class SpecialStoragePolicy;
}  // namespace storage

namespace content {

// Implements a PersistentCookieStore that deletes session cookies on
// shutdown. For documentation about the actual member functions consult the
// parent class |net::CookieMonster::PersistentCookieStore|. If provided, a
// |SpecialStoragePolicy| is consulted when the SQLite database is closed to
// decide which cookies to keep.
class CONTENT_EXPORT QuotaPolicyCookieStore
    : public net::CookieMonster::PersistentCookieStore {
 public:
  // Wraps the passed-in |cookie_store|.
  QuotaPolicyCookieStore(
      const scoped_refptr<net::SQLitePersistentCookieStore>& cookie_store,
      storage::SpecialStoragePolicy* special_storage_policy);

  // net::CookieMonster::PersistentCookieStore:
  void Load(const LoadedCallback& loaded_callback) override;
  void LoadCookiesForKey(const std::string& key,
                         const LoadedCallback& callback) override;
  void AddCookie(const net::CanonicalCookie& cc) override;
  void UpdateCookieAccessTime(const net::CanonicalCookie& cc) override;
  void DeleteCookie(const net::CanonicalCookie& cc) override;
  void SetForceKeepSessionState() override;
  void SetBeforeFlushCallback(base::RepeatingClosure callback) override;
  void Flush(base::OnceClosure callback) override;

 private:
  typedef std::map<net::SQLitePersistentCookieStore::CookieOrigin, size_t>
      CookiesPerOriginMap;

  ~QuotaPolicyCookieStore() override;

  // Called after cookies are loaded from the database.  Calls |loaded_callback|
  // when done.
  void OnLoad(const LoadedCallback& loaded_callback,
              std::vector<std::unique_ptr<net::CanonicalCookie>> cookies);

  // Map of (domain keys(eTLD+1), is secure cookie) to number of cookies in the
  // database.
  CookiesPerOriginMap cookies_per_origin_;

  scoped_refptr<storage::SpecialStoragePolicy> special_storage_policy_;
  scoped_refptr<net::SQLitePersistentCookieStore> persistent_store_;

  DISALLOW_COPY_AND_ASSIGN(QuotaPolicyCookieStore);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NET_QUOTA_POLICY_COOKIE_STORE_H_
