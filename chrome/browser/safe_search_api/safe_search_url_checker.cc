// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_search_api/safe_search_url_checker.h"

#include <string>
#include <utility>

#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/google/core/browser/google_util.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_request_status.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/url_constants.h"

namespace {

const char kSafeSearchApiUrl[] =
    "https://safesearch.googleapis.com/v1:classify";
const char kDataContentType[] = "application/x-www-form-urlencoded";
const char kDataFormat[] = "key=%s&urls=%s";

const size_t kDefaultCacheSize = 1000;
const size_t kDefaultCacheTimeoutSeconds = 3600;

// Builds the POST data for SafeSearch API requests.
std::string BuildRequestData(const std::string& api_key, const GURL& url) {
  std::string query = net::EscapeQueryParamValue(url.spec(), true);
  return base::StringPrintf(kDataFormat, api_key.c_str(), query.c_str());
}

// Parses a SafeSearch API |response| and stores the result in |is_porn|.
// On errors, returns false and doesn't set |is_porn|.
bool ParseResponse(const std::string& response, bool* is_porn) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(response);
  const base::DictionaryValue* dict = nullptr;
  if (!value || !value->GetAsDictionary(&dict)) {
    DLOG(WARNING) << "ParseResponse failed to parse global dictionary";
    return false;
  }
  const base::ListValue* classifications_list = nullptr;
  if (!dict->GetList("classifications", &classifications_list)) {
    DLOG(WARNING) << "ParseResponse failed to parse classifications list";
    return false;
  }
  if (classifications_list->GetSize() != 1) {
    DLOG(WARNING) << "ParseResponse expected exactly one result";
    return false;
  }
  const base::DictionaryValue* classification_dict = nullptr;
  if (!classifications_list->GetDictionary(0, &classification_dict)) {
    DLOG(WARNING) << "ParseResponse failed to parse classification dict";
    return false;
  }
  classification_dict->GetBoolean("pornography", is_porn);
  return true;
}

}  // namespace

struct SafeSearchURLChecker::Check {
  Check(const GURL& url,
        std::unique_ptr<network::SimpleURLLoader> simple_url_loader,
        CheckCallback callback);
  ~Check();

  GURL url;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader;
  std::vector<CheckCallback> callbacks;
  base::TimeTicks start_time;
};

SafeSearchURLChecker::Check::Check(
    const GURL& url,
    std::unique_ptr<network::SimpleURLLoader> simple_url_loader,
    CheckCallback callback)
    : url(url),
      simple_url_loader(std::move(simple_url_loader)),
      start_time(base::TimeTicks::Now()) {
  callbacks.push_back(std::move(callback));
}

SafeSearchURLChecker::Check::~Check() {
  for (const CheckCallback& callback : callbacks) {
    DCHECK(!callback);
  }
}

SafeSearchURLChecker::CheckResult::CheckResult(Classification classification,
                                               bool uncertain)
    : classification(classification),
      uncertain(uncertain),
      timestamp(base::TimeTicks::Now()) {}

SafeSearchURLChecker::SafeSearchURLChecker(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : SafeSearchURLChecker(std::move(url_loader_factory),
                           traffic_annotation,
                           kDefaultCacheSize) {}

SafeSearchURLChecker::SafeSearchURLChecker(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    size_t cache_size)
    : url_loader_factory_(std::move(url_loader_factory)),
      traffic_annotation_(traffic_annotation),
      cache_(cache_size),
      cache_timeout_(
          base::TimeDelta::FromSeconds(kDefaultCacheTimeoutSeconds)) {}

SafeSearchURLChecker::~SafeSearchURLChecker() {}

bool SafeSearchURLChecker::CheckURL(const GURL& url, CheckCallback callback) {
  // TODO(treib): Hack: For now, allow all Google URLs to save QPS. If we ever
  // remove this, we should find a way to allow at least the NTP.
  if (google_util::IsGoogleDomainUrl(url, google_util::ALLOW_SUBDOMAIN,
                                     google_util::ALLOW_NON_STANDARD_PORTS)) {
    std::move(callback).Run(url, Classification::SAFE, false);
    return true;
  }
  // TODO(treib): Hack: For now, allow all YouTube URLs since YouTube has its
  // own Safety Mode anyway.
  if (google_util::IsYoutubeDomainUrl(url, google_util::ALLOW_SUBDOMAIN,
                                      google_util::ALLOW_NON_STANDARD_PORTS)) {
    std::move(callback).Run(url, Classification::SAFE, false);
    return true;
  }

  auto cache_it = cache_.Get(url);
  if (cache_it != cache_.end()) {
    const CheckResult& result = cache_it->second;
    base::TimeDelta age = base::TimeTicks::Now() - result.timestamp;
    if (age < cache_timeout_) {
      DVLOG(1) << "Cache hit! " << url.spec() << " is "
               << (result.classification == Classification::UNSAFE ? "NOT" : "")
               << " safe; certain: " << !result.uncertain;
      std::move(callback).Run(url, result.classification, result.uncertain);
      return true;
    }
    DVLOG(1) << "Outdated cache entry for " << url.spec() << ", purging";
    cache_.Erase(cache_it);
  }

  // See if we already have a check in progress for this URL.
  for (const auto& check : checks_in_progress_) {
    if (check->url == url) {
      DVLOG(1) << "Adding to pending check for " << url.spec();
      check->callbacks.push_back(std::move(callback));
      return false;
    }
  }

  DVLOG(1) << "Checking URL " << url;
  std::string api_key = google_apis::GetAPIKey();
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(kSafeSearchApiUrl);
  resource_request->method = "POST";
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader =
      network::SimpleURLLoader::Create(std::move(resource_request),
                                       traffic_annotation_);
  simple_url_loader->AttachStringForUpload(BuildRequestData(api_key, url),
                                           kDataContentType);
  auto it = checks_in_progress_.insert(
      checks_in_progress_.begin(),
      std::make_unique<Check>(url, std::move(simple_url_loader),
                              std::move(callback)));
  network::SimpleURLLoader* loader = it->get()->simple_url_loader.get();
  loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&SafeSearchURLChecker::OnSimpleLoaderComplete,
                     base::Unretained(this), std::move(it)));
  return false;
}

void SafeSearchURLChecker::OnSimpleLoaderComplete(
    CheckList::iterator it,
    std::unique_ptr<std::string> response_body) {
  Check* check = it->get();

  GURL url = check->url;
  std::vector<CheckCallback> callbacks = std::move(check->callbacks);
  base::TimeTicks start_time = check->start_time;
  checks_in_progress_.erase(it);

  if (!response_body) {
    DLOG(WARNING) << "URL request failed! Letting through...";
    for (size_t i = 0; i < callbacks.size(); i++)
      std::move(callbacks[i]).Run(url, Classification::SAFE, true);
    return;
  }

  bool is_porn = false;
  bool uncertain = !ParseResponse(*response_body, &is_porn);
  Classification classification =
      is_porn ? Classification::UNSAFE : Classification::SAFE;

  // TODO(msramek): Consider moving this to SupervisedUserResourceThrottle.
  UMA_HISTOGRAM_TIMES("ManagedUsers.SafeSitesDelay",
                      base::TimeTicks::Now() - start_time);

  cache_.Put(url, CheckResult(classification, uncertain));

  for (size_t i = 0; i < callbacks.size(); i++)
    std::move(callbacks[i]).Run(url, classification, uncertain);
}
