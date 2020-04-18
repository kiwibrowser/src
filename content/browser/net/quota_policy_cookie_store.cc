// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/net/quota_policy_cookie_store.h"

#include <list>
#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_util.h"
#include "net/extras/sqlite/cookie_crypto_delegate.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "url/gurl.h"

namespace content {

QuotaPolicyCookieStore::QuotaPolicyCookieStore(
    const scoped_refptr<net::SQLitePersistentCookieStore>& cookie_store,
    storage::SpecialStoragePolicy* special_storage_policy)
    : special_storage_policy_(special_storage_policy),
      persistent_store_(cookie_store) {
}

QuotaPolicyCookieStore::~QuotaPolicyCookieStore() {
  using CookieOrigin = net::SQLitePersistentCookieStore::CookieOrigin;
  if (!special_storage_policy_.get() ||
      !special_storage_policy_->HasSessionOnlyOrigins()) {
    return;
  }

  std::list<CookieOrigin> session_only_cookies;
  auto delete_cookie_predicate =
      special_storage_policy_->CreateDeleteCookieOnExitPredicate();
  DCHECK(delete_cookie_predicate);

  for (const auto& entry : cookies_per_origin_) {
    if (entry.second == 0) {
      continue;
    }
    const CookieOrigin& cookie = entry.first;
    const GURL url(
        net::cookie_util::CookieOriginToURL(cookie.first, cookie.second));
    if (!url.is_valid() ||
        !delete_cookie_predicate.Run(cookie.first, cookie.second)) {
      continue;
    }
    session_only_cookies.push_back(cookie);
  }

  persistent_store_->DeleteAllInList(session_only_cookies);
}

void QuotaPolicyCookieStore::Load(const LoadedCallback& loaded_callback) {
  persistent_store_->Load(
      base::Bind(&QuotaPolicyCookieStore::OnLoad, this, loaded_callback));
}

void QuotaPolicyCookieStore::LoadCookiesForKey(
    const std::string& key,
    const LoadedCallback& loaded_callback) {
  persistent_store_->LoadCookiesForKey(
      key,
      base::Bind(&QuotaPolicyCookieStore::OnLoad, this, loaded_callback));
}

void QuotaPolicyCookieStore::AddCookie(const net::CanonicalCookie& cc) {
  net::SQLitePersistentCookieStore::CookieOrigin origin(
      cc.Domain(), cc.IsSecure());
  ++cookies_per_origin_[origin];
  persistent_store_->AddCookie(cc);
}

void QuotaPolicyCookieStore::UpdateCookieAccessTime(
    const net::CanonicalCookie& cc) {
  persistent_store_->UpdateCookieAccessTime(cc);
}

void QuotaPolicyCookieStore::DeleteCookie(const net::CanonicalCookie& cc) {
  net::SQLitePersistentCookieStore::CookieOrigin origin(
      cc.Domain(), cc.IsSecure());
  DCHECK_GE(cookies_per_origin_[origin], 1U);
  --cookies_per_origin_[origin];
  persistent_store_->DeleteCookie(cc);
}

void QuotaPolicyCookieStore::SetForceKeepSessionState() {
  special_storage_policy_ = nullptr;
}

void QuotaPolicyCookieStore::SetBeforeFlushCallback(
    base::RepeatingClosure callback) {
  persistent_store_->SetBeforeFlushCallback(std::move(callback));
}

void QuotaPolicyCookieStore::Flush(base::OnceClosure callback) {
  persistent_store_->Flush(std::move(callback));
}

void QuotaPolicyCookieStore::OnLoad(
    const LoadedCallback& loaded_callback,
    std::vector<std::unique_ptr<net::CanonicalCookie>> cookies) {
  for (const auto& cookie : cookies) {
    net::SQLitePersistentCookieStore::CookieOrigin origin(
        cookie->Domain(), cookie->IsSecure());
    ++cookies_per_origin_[origin];
  }

  loaded_callback.Run(std::move(cookies));
}

CookieStoreConfig::CookieStoreConfig()
    : restore_old_session_cookies(false),
      persist_session_cookies(false),
      crypto_delegate(nullptr),
      channel_id_service(nullptr) {
  // Default to an in-memory cookie store.
}

CookieStoreConfig::CookieStoreConfig(
    const base::FilePath& path,
    bool restore_old_session_cookies,
    bool persist_session_cookies,
    storage::SpecialStoragePolicy* storage_policy)
    : path(path),
      restore_old_session_cookies(restore_old_session_cookies),
      persist_session_cookies(persist_session_cookies),
      storage_policy(storage_policy),
      crypto_delegate(nullptr),
      channel_id_service(nullptr) {
  CHECK(!path.empty() ||
        (!restore_old_session_cookies && !persist_session_cookies));
}

CookieStoreConfig::~CookieStoreConfig() {
}

std::unique_ptr<net::CookieStore> CreateCookieStore(
    const CookieStoreConfig& config) {
  std::unique_ptr<net::CookieMonster> cookie_monster;

  if (config.path.empty()) {
    // Empty path means in-memory store.
    cookie_monster.reset(new net::CookieMonster(nullptr));
  } else {
    scoped_refptr<base::SequencedTaskRunner> client_task_runner =
        config.client_task_runner;
    scoped_refptr<base::SequencedTaskRunner> background_task_runner =
        config.background_task_runner;

    if (!client_task_runner.get()) {
      client_task_runner =
          BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
    }

    if (!background_task_runner.get()) {
      background_task_runner = base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
    }

    scoped_refptr<net::SQLitePersistentCookieStore> sqlite_store(
        new net::SQLitePersistentCookieStore(
            config.path, client_task_runner, background_task_runner,
            config.restore_old_session_cookies, config.crypto_delegate));

    QuotaPolicyCookieStore* persistent_store =
        new QuotaPolicyCookieStore(
            sqlite_store.get(),
            config.storage_policy.get());

    cookie_monster.reset(new net::CookieMonster(persistent_store,
                                                config.channel_id_service));
    if (config.persist_session_cookies)
      cookie_monster->SetPersistSessionCookies(true);
  }

  if (!config.cookieable_schemes.empty())
    cookie_monster->SetCookieableSchemes(config.cookieable_schemes);

  return std::move(cookie_monster);
}

}  // namespace content
