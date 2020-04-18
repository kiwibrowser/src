// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_TILES_POPULAR_SITES_IMPL_H_
#define COMPONENTS_NTP_TILES_POPULAR_SITES_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "components/ntp_tiles/popular_sites.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace base {
class Value;
}

namespace net {
class URLRequestContextGetter;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace variations {
class VariationsService;
}

class PrefService;
class TemplateURLService;

namespace ntp_tiles {

using ParseJSONCallback = base::Callback<void(
    const std::string& unsafe_json,
    const base::Callback<void(std::unique_ptr<base::Value>)>& success_callback,
    const base::Callback<void(const std::string&)>& error_callback)>;

// Actual (non-test) implementation of the PopularSites interface. Caches the
// downloaded file on disk to avoid re-downloading on every startup.
class PopularSitesImpl : public PopularSites, public net::URLFetcherDelegate {
 public:
  PopularSitesImpl(
      PrefService* prefs,
      const TemplateURLService* template_url_service,
      variations::VariationsService* variations_service,
      net::URLRequestContextGetter* download_context,
      ParseJSONCallback parse_json);

  ~PopularSitesImpl() override;

  // PopularSites implementation.
  bool MaybeStartFetch(bool force_download,
                       const FinishedCallback& callback) override;
  const std::map<SectionType, SitesVector>& sections() const override;
  GURL GetLastURLFetched() const override;
  GURL GetURLToFetch() override;
  std::string GetDirectoryToFetch() override;
  std::string GetCountryToFetch() override;
  std::string GetVersionToFetch() override;
  const base::ListValue* GetCachedJson() override;

  // Register preferences used by this class.
  static void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* user_prefs);

 private:
  // Fetch the popular sites at the given URL, overwriting any cache in prefs
  // that already exists.
  void FetchPopularSites();

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void OnJsonParsed(std::unique_ptr<base::Value> json);
  void OnJsonParseFailed(const std::string& error_message);
  void OnDownloadFailed();

  // Parameters set from constructor.
  PrefService* const prefs_;
  const TemplateURLService* const template_url_service_;
  variations::VariationsService* const variations_;
  net::URLRequestContextGetter* const download_context_;
  ParseJSONCallback parse_json_;

  // Set by MaybeStartFetch() and called after fetch completes.
  FinishedCallback callback_;

  std::unique_ptr<net::URLFetcher> fetcher_;
  bool is_fallback_;
  std::map<SectionType, SitesVector> sections_;
  GURL pending_url_;
  int version_in_pending_url_;

  base::WeakPtrFactory<PopularSitesImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PopularSitesImpl);
};

}  // namespace ntp_tiles

#endif  // COMPONENTS_NTP_TILES_POPULAR_SITES_IMPL_H_
