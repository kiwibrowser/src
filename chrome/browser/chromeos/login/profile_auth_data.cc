// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/profile_auth_data.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_store.h"
#include "net/http/http_auth_cache.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/channel_id_store.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace chromeos {

namespace {

const char kSAMLStartCookie[] = "google-accounts-saml-start";
const char kSAMLEndCookie[] = "google-accounts-saml-end";

// Import |cookies| into |cookie_store|.
// |cookie.IsCanonical()| must be true for all cookies in |cookies|.
void ImportCookies(const net::CookieList& cookies,
                   net::CookieStore* cookie_store) {
  for (const auto& cookie : cookies) {
    std::unique_ptr<net::CanonicalCookie> cc(
        std::make_unique<net::CanonicalCookie>(cookie));
    DCHECK(cc->IsCanonical());

    // Assume secure_source - since the cookies are being restored from
    // another store, they have already gone through the strict secure check.
    cookie_store->SetCanonicalCookieAsync(
        std::move(cc), true /*secure_source*/, true /*modify_http_only*/,
        net::CookieStore::SetCookiesCallback());
  }
}

class ProfileAuthDataTransferer {
 public:
  ProfileAuthDataTransferer(
      net::URLRequestContextGetter* from_context,
      net::URLRequestContextGetter* to_context,
      bool transfer_auth_cookies_and_channel_ids_on_first_login,
      bool transfer_saml_auth_cookies_on_subsequent_login,
      const base::Closure& completion_callback);

  void BeginTransfer();

 private:
  void BeginTransferOnIOThread();

  // Transfer the proxy auth cache from |from_context_| to |to_context_|. If
  // the user was required to authenticate with a proxy during login, this
  // authentication information will be transferred into the user's session.
  void TransferProxyAuthCache();

  // Callback that receives the content of |to_context_|'s cookie jar. Checks
  // whether this is the user's first login, based on the state of the cookie
  // jar, and starts retrieval of the data that should be transfered. Calls
  // Finish() if there is no data to transfer.
  void OnTargetCookieJarContentsRetrieved(
      const net::CookieList& target_cookies);

  // Retrieve the contents of |from_context_|'s cookie jar. When the retrieval
  // finishes, OnCookiesToTransferRetrieved will be called with the result.
  void RetrieveCookiesToTransfer();

  // Callback that receives the contents of |from_context_|'s cookie jar. Calls
  // MaybeTransferCookiesAndChannelIDs() to try and perform the transfer.
  void OnCookiesToTransferRetrieved(const net::CookieList& cookies_to_transfer);

  // Retrieve |from_context_|'s channel IDs. When the retrieval finishes,
  // OnChannelIDsToTransferRetrieved will be called with the result.
  void RetrieveChannelIDsToTransfer();

  // Callback that receives |from_context_|'s channel IDs. Calls
  // MaybeTransferCookiesAndChannelIDs() to try and perform the transfer.
  void OnChannelIDsToTransferRetrieved(
      const net::ChannelIDStore::ChannelIDList& channel_ids_to_transfer);

  // Given a |cookie| set during login, returns true if the cookie may have been
  // set by GAIA. The main criterion is the |cookie|'s creation date. The points
  // in time at which redirects from GAIA to SAML IdP and back occur are stored
  // in |saml_start_time_| and |saml_end_time_|. If the cookie was set between
  // these two times, it was created by the SAML IdP. Otherwise, it was created
  // by GAIA.
  // As an additional precaution, the cookie's domain is checked. If the domain
  // contains "google" or "youtube", the cookie is considered to have been set
  // by GAIA as well.
  bool IsGAIACookie(const net::CanonicalCookie& cookie);

  // If all data to be transferred has been retrieved already, transfer it to
  // |to_context_| and call Finish().
  void MaybeTransferCookiesAndChannelIDs();

  // Post the |completion_callback_| to the UI thread and schedule destruction
  // of |this|.
  void Finish();

  scoped_refptr<net::URLRequestContextGetter> from_context_;
  scoped_refptr<net::URLRequestContextGetter> to_context_;
  bool transfer_auth_cookies_and_channel_ids_on_first_login_;
  bool transfer_saml_auth_cookies_on_subsequent_login_;
  base::Closure completion_callback_;

  net::CookieList cookies_to_transfer_;
  net::ChannelIDStore::ChannelIDList channel_ids_to_transfer_;

  // The time at which a redirect from GAIA to a SAML IdP occurred.
  base::Time saml_start_time_;
  // The time at which a redirect from a SAML IdP back to GAIA occurred.
  base::Time saml_end_time_;

  bool first_login_;
  bool waiting_for_auth_cookies_;
  bool waiting_for_channel_ids_;
};

ProfileAuthDataTransferer::ProfileAuthDataTransferer(
    net::URLRequestContextGetter* from_context,
    net::URLRequestContextGetter* to_context,
    bool transfer_auth_cookies_and_channel_ids_on_first_login,
    bool transfer_saml_auth_cookies_on_subsequent_login,
    const base::Closure& completion_callback)
    : from_context_(from_context),
      to_context_(to_context),
      transfer_auth_cookies_and_channel_ids_on_first_login_(
          transfer_auth_cookies_and_channel_ids_on_first_login),
      transfer_saml_auth_cookies_on_subsequent_login_(
          transfer_saml_auth_cookies_on_subsequent_login),
      completion_callback_(completion_callback),
      first_login_(false),
      waiting_for_auth_cookies_(false),
      waiting_for_channel_ids_(false) {}

void ProfileAuthDataTransferer::BeginTransfer() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // If we aren't transferring auth cookies or channel IDs, post the completion
  // callback immediately. Otherwise, it will be called when the transfer
  // finishes.
  if (!transfer_auth_cookies_and_channel_ids_on_first_login_ &&
      !transfer_saml_auth_cookies_on_subsequent_login_) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, completion_callback_);
    // Null the callback so that when Finish is called, the callback won't be
    // called again.
    completion_callback_.Reset();
  }
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&ProfileAuthDataTransferer::BeginTransferOnIOThread,
                     base::Unretained(this)));
}

void ProfileAuthDataTransferer::BeginTransferOnIOThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  TransferProxyAuthCache();
  if (transfer_auth_cookies_and_channel_ids_on_first_login_ ||
      transfer_saml_auth_cookies_on_subsequent_login_) {
    // Retrieve the contents of |to_context_|'s cookie jar.
    net::CookieStore* to_store =
        to_context_->GetURLRequestContext()->cookie_store();
    to_store->GetAllCookiesAsync(base::BindOnce(
        &ProfileAuthDataTransferer::OnTargetCookieJarContentsRetrieved,
        base::Unretained(this)));
  } else {
    Finish();
  }
}

void ProfileAuthDataTransferer::TransferProxyAuthCache() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  net::HttpAuthCache* new_cache = to_context_->GetURLRequestContext()
                                      ->http_transaction_factory()
                                      ->GetSession()
                                      ->http_auth_cache();
  new_cache->UpdateAllFrom(*from_context_->GetURLRequestContext()
                                ->http_transaction_factory()
                                ->GetSession()
                                ->http_auth_cache());
}

void ProfileAuthDataTransferer::OnTargetCookieJarContentsRetrieved(
    const net::CookieList& target_cookies) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  first_login_ = target_cookies.empty();
  if (first_login_) {
    // On first login, transfer all auth cookies and channel IDs if
    // |transfer_auth_cookies_and_channel_ids_on_first_login_| is true.
    waiting_for_auth_cookies_ =
        transfer_auth_cookies_and_channel_ids_on_first_login_;
    waiting_for_channel_ids_ =
        transfer_auth_cookies_and_channel_ids_on_first_login_;
  } else {
    // On subsequent login, transfer auth cookies set by the SAML IdP if
    // |transfer_saml_auth_cookies_on_subsequent_login_| is true.
    waiting_for_auth_cookies_ = transfer_saml_auth_cookies_on_subsequent_login_;
  }

  if (!waiting_for_auth_cookies_ && !waiting_for_channel_ids_) {
    Finish();
    return;
  }

  if (waiting_for_auth_cookies_)
    RetrieveCookiesToTransfer();
  if (waiting_for_channel_ids_)
    RetrieveChannelIDsToTransfer();
}

void ProfileAuthDataTransferer::RetrieveCookiesToTransfer() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  net::CookieStore* from_store =
      from_context_->GetURLRequestContext()->cookie_store();
  from_store->GetAllCookiesAsync(
      base::BindOnce(&ProfileAuthDataTransferer::OnCookiesToTransferRetrieved,
                     base::Unretained(this)));
}

void ProfileAuthDataTransferer::OnCookiesToTransferRetrieved(
    const net::CookieList& cookies_to_transfer) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  waiting_for_auth_cookies_ = false;
  cookies_to_transfer_ = cookies_to_transfer;

  // Look for cookies indicating the points in time at which redirects from GAIA
  // to SAML IdP and back occurred. These cookies are synthesized by
  // chrome/browser/resources/gaia_auth/background.js. If the cookies are found,
  // their creation times are stored in |saml_start_time_| and
  // |cookies_to_transfer_| and the cookies are deleted.
  for (net::CookieList::iterator it = cookies_to_transfer_.begin();
       it != cookies_to_transfer_.end();) {
    if (it->Name() == kSAMLStartCookie) {
      saml_start_time_ = it->CreationDate();
      it = cookies_to_transfer_.erase(it);
    } else if (it->Name() == kSAMLEndCookie) {
      saml_end_time_ = it->CreationDate();
      it = cookies_to_transfer_.erase(it);
    } else {
      ++it;
    }
  }

  MaybeTransferCookiesAndChannelIDs();
}

void ProfileAuthDataTransferer::RetrieveChannelIDsToTransfer() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  net::ChannelIDService* from_service =
      from_context_->GetURLRequestContext()->channel_id_service();
  from_service->GetChannelIDStore()->GetAllChannelIDs(
      base::Bind(&ProfileAuthDataTransferer::OnChannelIDsToTransferRetrieved,
                 base::Unretained(this)));
}

void ProfileAuthDataTransferer::OnChannelIDsToTransferRetrieved(
    const net::ChannelIDStore::ChannelIDList& channel_ids_to_transfer) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  channel_ids_to_transfer_ = channel_ids_to_transfer;
  waiting_for_channel_ids_ = false;
  MaybeTransferCookiesAndChannelIDs();
}

bool ProfileAuthDataTransferer::IsGAIACookie(
    const net::CanonicalCookie& cookie) {
  const base::Time& creation_date = cookie.CreationDate();
  if (creation_date < saml_start_time_)
    return true;
  if (!saml_end_time_.is_null() && creation_date > saml_end_time_)
    return true;

  const std::string& domain = cookie.Domain();
  return domain.find("google") != std::string::npos ||
         domain.find("youtube") != std::string::npos;
}

void ProfileAuthDataTransferer::MaybeTransferCookiesAndChannelIDs() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (waiting_for_auth_cookies_ || waiting_for_channel_ids_)
    return;

  net::CookieStore* to_store =
      to_context_->GetURLRequestContext()->cookie_store();
  if (first_login_) {
    ImportCookies(cookies_to_transfer_, to_store);
    net::ChannelIDService* to_cert_service =
        to_context_->GetURLRequestContext()->channel_id_service();
    to_cert_service->GetChannelIDStore()->InitializeFrom(
        channel_ids_to_transfer_);
  } else {
    net::CookieList non_gaia_cookies;
    for (net::CookieList::const_iterator it = cookies_to_transfer_.begin();
         it != cookies_to_transfer_.end(); ++it) {
      if (!IsGAIACookie(*it))
        non_gaia_cookies.push_back(*it);
    }
    ImportCookies(non_gaia_cookies, to_store);
  }

  Finish();
}

void ProfileAuthDataTransferer::Finish() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!completion_callback_.is_null())
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, completion_callback_);
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

}  // namespace

void ProfileAuthData::Transfer(
    net::URLRequestContextGetter* from_context,
    net::URLRequestContextGetter* to_context,
    bool transfer_auth_cookies_and_channel_ids_on_first_login,
    bool transfer_saml_auth_cookies_on_subsequent_login,
    const base::Closure& completion_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  (new ProfileAuthDataTransferer(
       from_context, to_context,
       transfer_auth_cookies_and_channel_ids_on_first_login,
       transfer_saml_auth_cookies_on_subsequent_login, completion_callback))
      ->BeginTransfer();
}

}  // namespace chromeos
