// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/local_storage_usage_info.h"
#include "content/public/browser/storage_partition.h"
#include "net/cookies/cookie_util.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "storage/browser/quota/special_storage_policy.h"

namespace {

class SessionDataDeleter
    : public base::RefCountedThreadSafe<SessionDataDeleter> {
 public:
  SessionDataDeleter(storage::SpecialStoragePolicy* storage_policy,
                     bool delete_only_by_session_only_policy);

  void Run(content::StoragePartition* storage_partition);

 private:
  friend class base::RefCountedThreadSafe<SessionDataDeleter>;
  ~SessionDataDeleter();

  // Deletes the local storage described by |usages| for origins which are
  // session-only.
  void ClearSessionOnlyLocalStorage(
      content::StoragePartition* storage_partition,
      const std::vector<content::LocalStorageUsageInfo>& usages);

  // Takes the result of a CookieManager::GetAllCookies() method, and
  // initiates deletion of all cookies that are session only by the
  // storage policy of the constructor.
  void DeleteSessionOnlyOriginCookies(
      const std::vector<net::CanonicalCookie>& cookies);

  network::mojom::CookieManagerPtr cookie_manager_;
  scoped_refptr<storage::SpecialStoragePolicy> storage_policy_;
  const bool delete_only_by_session_only_policy_;

  DISALLOW_COPY_AND_ASSIGN(SessionDataDeleter);
};

SessionDataDeleter::SessionDataDeleter(
    storage::SpecialStoragePolicy* storage_policy,
    bool delete_only_by_session_only_policy)
    : storage_policy_(storage_policy),
      delete_only_by_session_only_policy_(delete_only_by_session_only_policy) {
}

void SessionDataDeleter::Run(content::StoragePartition* storage_partition) {
  if (storage_policy_.get() && storage_policy_->HasSessionOnlyOrigins()) {
    storage_partition->GetDOMStorageContext()->GetLocalStorageUsage(
        base::Bind(&SessionDataDeleter::ClearSessionOnlyLocalStorage,
                   this,
                   storage_partition));
  }

  storage_partition->GetNetworkContext()->GetCookieManager(
      mojo::MakeRequest(&cookie_manager_));

  if (!delete_only_by_session_only_policy_) {
    network::mojom::CookieDeletionFilterPtr filter(
        network::mojom::CookieDeletionFilter::New());
    filter->session_control =
        network::mojom::CookieDeletionSessionControl::SESSION_COOKIES;
    cookie_manager_->DeleteCookies(
        std::move(filter),
        // Fire and forget
        network::mojom::CookieManager::DeleteCookiesCallback());
  }

  if (!storage_policy_.get() || !storage_policy_->HasSessionOnlyOrigins())
    return;

  cookie_manager_->GetAllCookies(
      base::Bind(&SessionDataDeleter::DeleteSessionOnlyOriginCookies, this));
  // Note that from this point on |*this| is kept alive by scoped_refptr<>
  // references automatically taken by |Bind()|, so when the last callback
  // created by Bind() is released (after execution of that function), the
  // object will be deleted.  This may result in any callbacks passed to
  // |*cookie_manager_.get()| methods not being executed because of the
  // destruction of the CookieManagerPtr, but the model of the
  // SessionDataDeleter is "fire-and-forget" and all such callbacks are
  // empty, so that is ok.  Mojo guarantees that all messages pushed onto a
  // pipe will be executed by the server side of the pipe even if the client
  // side of the pipe is closed, so all deletion requested will still occur.
}

void SessionDataDeleter::DeleteSessionOnlyOriginCookies(
    const std::vector<net::CanonicalCookie>& cookies) {
  base::Time yesterday(base::Time::Now() - base::TimeDelta::FromDays(1));

  auto delete_cookie_predicate =
      storage_policy_->CreateDeleteCookieOnExitPredicate();
  DCHECK(delete_cookie_predicate);

  for (const auto& cookie : cookies) {
    if (!delete_cookie_predicate.Run(cookie.Domain(), cookie.IsSecure())) {
      continue;
    }

    // Delete a single cookie by setting its expiration time into the past.
    cookie_manager_->SetCanonicalCookie(
        net::CanonicalCookie(cookie.Name(), cookie.Value(), cookie.Domain(),
                             cookie.Path(), cookie.CreationDate(), yesterday,
                             cookie.LastAccessDate(), cookie.IsSecure(),
                             cookie.IsHttpOnly(), cookie.SameSite(),
                             cookie.Priority()),
        true /* secure_source */, true /* modify_http_only */,
        // Fire and forget
        network::mojom::CookieManager::SetCanonicalCookieCallback());
  }
}

SessionDataDeleter::~SessionDataDeleter() {}

void SessionDataDeleter::ClearSessionOnlyLocalStorage(
    content::StoragePartition* storage_partition,
    const std::vector<content::LocalStorageUsageInfo>& usages) {
  DCHECK(storage_policy_.get());
  DCHECK(storage_policy_->HasSessionOnlyOrigins());
  for (size_t i = 0; i < usages.size(); ++i) {
    const content::LocalStorageUsageInfo& usage = usages[i];
    if (!storage_policy_->IsStorageSessionOnly(usage.origin))
      continue;
    storage_partition->GetDOMStorageContext()->DeleteLocalStorage(
        usage.origin, base::DoNothing());
  }
}

}  // namespace

void DeleteSessionOnlyData(Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (browser_shutdown::IsTryingToQuit())
    return;

  // TODO: Remove Athena special casing once the AthenaSessionRestore is in
  // place.
#if defined(OS_ANDROID)
  SessionStartupPref::Type startup_pref_type =
      SessionStartupPref::GetDefaultStartupType();
#else
  SessionStartupPref::Type startup_pref_type =
      StartupBrowserCreator::GetSessionStartupPref(
          *base::CommandLine::ForCurrentProcess(), profile).type;
#endif

  scoped_refptr<SessionDataDeleter> deleter(
      new SessionDataDeleter(profile->GetSpecialStoragePolicy(),
                             startup_pref_type == SessionStartupPref::LAST));
  deleter->Run(Profile::GetDefaultStoragePartition(profile));
}
