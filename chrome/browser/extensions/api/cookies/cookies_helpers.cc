// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implements common functionality for the Chrome Extensions Cookies API.

#include "chrome/browser/extensions/api/cookies/cookies_helpers.h"

#include <stddef.h>

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/cookies/cookies_api_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/extensions/api/cookies.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/permissions_data.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_store.h"
#include "net/cookies/cookie_util.h"
#include "url/gurl.h"

using extensions::api::cookies::Cookie;
using extensions::api::cookies::CookieStore;

namespace GetAll = extensions::api::cookies::GetAll;

namespace extensions {

namespace keys = cookies_api_constants;

namespace cookies_helpers {

static const char kOriginalProfileStoreId[] = "0";
static const char kOffTheRecordProfileStoreId[] = "1";

Profile* ChooseProfileFromStoreId(const std::string& store_id,
                                  Profile* profile,
                                  bool include_incognito) {
  DCHECK(profile);
  bool allow_original = !profile->IsOffTheRecord();
  bool allow_incognito = profile->IsOffTheRecord() ||
      (include_incognito && profile->HasOffTheRecordProfile());
  if (store_id == kOriginalProfileStoreId && allow_original)
    return profile->GetOriginalProfile();
  if (store_id == kOffTheRecordProfileStoreId && allow_incognito)
    return profile->GetOffTheRecordProfile();
  return NULL;
}

const char* GetStoreIdFromProfile(Profile* profile) {
  DCHECK(profile);
  return profile->IsOffTheRecord() ?
      kOffTheRecordProfileStoreId : kOriginalProfileStoreId;
}

Cookie CreateCookie(const net::CanonicalCookie& canonical_cookie,
                    const std::string& store_id) {
  Cookie cookie;

  // A cookie is a raw byte sequence. By explicitly parsing it as UTF-8, we
  // apply error correction, so the string can be safely passed to the renderer.
  cookie.name = base::UTF16ToUTF8(base::UTF8ToUTF16(canonical_cookie.Name()));
  cookie.value = base::UTF16ToUTF8(base::UTF8ToUTF16(canonical_cookie.Value()));
  cookie.domain = canonical_cookie.Domain();
  cookie.host_only =
      net::cookie_util::DomainIsHostOnly(canonical_cookie.Domain());
  // A non-UTF8 path is invalid, so we just replace it with an empty string.
  cookie.path = base::IsStringUTF8(canonical_cookie.Path())
                    ? canonical_cookie.Path()
                    : std::string();
  cookie.secure = canonical_cookie.IsSecure();
  cookie.http_only = canonical_cookie.IsHttpOnly();

  switch (canonical_cookie.SameSite()) {
  case net::CookieSameSite::DEFAULT_MODE:
    cookie.same_site = api::cookies::SAME_SITE_STATUS_NO_RESTRICTION;
    break;
  case net::CookieSameSite::LAX_MODE:
    cookie.same_site = api::cookies::SAME_SITE_STATUS_LAX;
    break;
  case net::CookieSameSite::STRICT_MODE:
    cookie.same_site = api::cookies::SAME_SITE_STATUS_STRICT;
    break;
  }

  cookie.session = !canonical_cookie.IsPersistent();
  if (canonical_cookie.IsPersistent()) {
    cookie.expiration_date.reset(
        new double(canonical_cookie.ExpiryDate().ToDoubleT()));
  }
  cookie.store_id = store_id;

  return cookie;
}

CookieStore CreateCookieStore(Profile* profile,
                              std::unique_ptr<base::ListValue> tab_ids) {
  DCHECK(profile);
  DCHECK(tab_ids);
  base::DictionaryValue dict;
  dict.SetString(keys::kIdKey, GetStoreIdFromProfile(profile));
  dict.Set(keys::kTabIdsKey, std::move(tab_ids));

  CookieStore cookie_store;
  bool rv = CookieStore::Populate(dict, &cookie_store);
  CHECK(rv);
  return cookie_store;
}

void GetCookieListFromManager(
    network::mojom::CookieManager* manager,
    const GURL& url,
    network::mojom::CookieManager::GetCookieListCallback callback) {
  if (url.is_empty()) {
    manager->GetAllCookies(std::move(callback));
  } else {
    net::CookieOptions options;
    options.set_include_httponly();
    options.set_same_site_cookie_mode(
        net::CookieOptions::SameSiteCookieMode::INCLUDE_STRICT_AND_LAX);
    options.set_do_not_update_access_time();

    manager->GetCookieList(url, options, std::move(callback));
  }
}

GURL GetURLFromCanonicalCookie(const net::CanonicalCookie& cookie) {
  const std::string& domain_key = cookie.Domain();
  const std::string scheme =
      cookie.IsSecure() ? url::kHttpsScheme : url::kHttpScheme;
  const std::string host =
      base::StartsWith(domain_key, ".", base::CompareCase::SENSITIVE)
          ? domain_key.substr(1)
          : domain_key;
  return GURL(scheme + url::kStandardSchemeSeparator + host + "/");
}

void AppendMatchingCookiesToVector(const net::CookieList& all_cookies,
                                   const GURL& url,
                                   const GetAll::Params::Details* details,
                                   const Extension* extension,
                                   std::vector<Cookie>* match_vector) {
  for (const net::CanonicalCookie& cookie : all_cookies) {
    // Ignore any cookie whose domain doesn't match the extension's
    // host permissions.
    GURL cookie_domain_url = GetURLFromCanonicalCookie(cookie);
    if (!extension->permissions_data()->HasHostPermission(cookie_domain_url))
      continue;
    // Filter the cookie using the match filter.
    cookies_helpers::MatchFilter filter(details);
    if (filter.MatchesCookie(cookie))
      match_vector->push_back(CreateCookie(cookie, *details->store_id));
  }
}

void AppendToTabIdList(Browser* browser, base::ListValue* tab_ids) {
  DCHECK(browser);
  DCHECK(tab_ids);
  TabStripModel* tab_strip = browser->tab_strip_model();
  for (int i = 0; i < tab_strip->count(); ++i) {
    tab_ids->AppendInteger(
        ExtensionTabUtil::GetTabId(tab_strip->GetWebContentsAt(i)));
  }
}

MatchFilter::MatchFilter(const GetAll::Params::Details* details)
    : details_(details) {
  DCHECK(details_);
}

bool MatchFilter::MatchesCookie(
    const net::CanonicalCookie& cookie) {
  if (details_->name.get() && *details_->name != cookie.Name())
    return false;

  if (!MatchesDomain(cookie.Domain()))
    return false;

  if (details_->path.get() && *details_->path != cookie.Path())
    return false;

  if (details_->secure.get() && *details_->secure != cookie.IsSecure())
    return false;

  if (details_->session.get() && *details_->session != !cookie.IsPersistent())
    return false;

  return true;
}

bool MatchFilter::MatchesDomain(const std::string& domain) {
  if (!details_->domain.get())
    return true;

  // Add a leading '.' character to the filter domain if it doesn't exist.
  if (net::cookie_util::DomainIsHostOnly(*details_->domain))
    details_->domain->insert(0, ".");

  std::string sub_domain(domain);
  // Strip any leading '.' character from the input cookie domain.
  if (!net::cookie_util::DomainIsHostOnly(sub_domain))
    sub_domain = sub_domain.substr(1);

  // Now check whether the domain argument is a subdomain of the filter domain.
  for (sub_domain.insert(0, ".");
       sub_domain.length() >= details_->domain->length();) {
    if (sub_domain == *details_->domain)
      return true;
    const size_t next_dot = sub_domain.find('.', 1);  // Skip over leading dot.
    sub_domain.erase(0, next_dot);
  }
  return false;
}

}  // namespace cookies_helpers
}  // namespace extensions
