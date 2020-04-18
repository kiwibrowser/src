// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/common/webservice_cache.h"

#include <stddef.h>
#include <stdint.h>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"

namespace app_list {
namespace {

const unsigned int kWebserviceCacheMaxSize = 1000;
const unsigned int kWebserviceCacheTimeLimitInMinutes = 1;

const char kKeyResultTime[] = "time";
const char kKeyResult[] = "result";

const char kWebstoreQueryPrefix[] = "webstore:";
const char kPeopleQueryPrefix[] = "people:";

}  // namespace

WebserviceCache::WebserviceCache(content::BrowserContext* context)
    : cache_(Cache::NO_AUTO_EVICT),
      cache_loaded_(false) {
  const char kStoreDataFileName[] = "Webservice Search Cache";
  const base::FilePath data_file =
      context->GetPath().AppendASCII(kStoreDataFileName);
  data_store_ = new DictionaryDataStore(data_file);
  data_store_->Load(base::Bind(&WebserviceCache::OnCacheLoaded, AsWeakPtr()));
}

WebserviceCache::~WebserviceCache() {
}

const CacheResult WebserviceCache::Get(QueryType type,
                                       const std::string& query) {
  std::string typed_query = PrependType(type, query);
  Cache::iterator iter = cache_.Get(typed_query);
  if (iter != cache_.end()) {
    if (base::Time::Now() - iter->second->time <=
        base::TimeDelta::FromMinutes(kWebserviceCacheTimeLimitInMinutes)) {
      return std::make_pair(FRESH, iter->second->result.get());
    } else {
      return std::make_pair(STALE, iter->second->result.get());
    }
  }
  return std::make_pair(STALE, static_cast<base::DictionaryValue*>(NULL));
}

void WebserviceCache::Put(QueryType type,
                          const std::string& query,
                          std::unique_ptr<base::DictionaryValue> result) {
  if (result) {
    std::string typed_query = PrependType(type, query);
    std::unique_ptr<Payload> scoped_payload(
        new Payload(base::Time::Now(), std::move(result)));
    Payload* payload = scoped_payload.get();

    cache_.Put(typed_query, std::move(scoped_payload));
    // If the cache isn't loaded yet, we're fine with losing queries since
    // a 1000 entry cache should load really quickly so the chance of a user
    // already having typed a 3 character search before the cache has loaded is
    // very unlikely.
    if (cache_loaded_) {
      data_store_->cached_dict()->Set(typed_query, DictFromPayload(*payload));
      data_store_->ScheduleWrite();
      if (cache_.size() > kWebserviceCacheMaxSize)
        TrimCache();
    }
  }
}

void WebserviceCache::OnCacheLoaded(std::unique_ptr<base::DictionaryValue>) {
  if (!data_store_->cached_dict())
    return;

  std::vector<std::string> cleanup_keys;
  for (base::DictionaryValue::Iterator it(*data_store_->cached_dict());
      !it.IsAtEnd();
      it.Advance()) {
    const base::DictionaryValue* payload_dict;
    std::unique_ptr<Payload> payload(new Payload);
    if (!it.value().GetAsDictionary(&payload_dict) ||
        !payload_dict ||
        !PayloadFromDict(payload_dict, payload.get())) {
      // In case we don't have a valid payload associated with a given query,
      // clean up that query from our data store.
      cleanup_keys.push_back(it.key());
      continue;
    }
    cache_.Put(it.key(), std::move(payload));
  }

  if (!cleanup_keys.empty()) {
    for (size_t i = 0; i < cleanup_keys.size(); ++i)
      data_store_->cached_dict()->Remove(cleanup_keys[i], NULL);
    data_store_->ScheduleWrite();
  }
  cache_loaded_ = true;
}

bool WebserviceCache::PayloadFromDict(const base::DictionaryValue* dict,
                                      Payload* payload) {
  std::string time_string;
  if (!dict->GetString(kKeyResultTime, &time_string))
    return false;
  const base::DictionaryValue* result;
  if (!dict->GetDictionary(kKeyResult, &result))
    return false;

  int64_t time_val;
  base::StringToInt64(time_string, &time_val);

  // The result dictionary will be owned by the cache, hence create a copy
  // instead of returning the original reference. The new dictionary will be
  // owned by our MRU cache.
  *payload = Payload(base::Time::FromInternalValue(time_val),
                     base::WrapUnique(result->DeepCopy()));
  return true;
}

std::unique_ptr<base::DictionaryValue> WebserviceCache::DictFromPayload(
    const Payload& payload) {
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetString(kKeyResultTime, base::Int64ToString(
      payload.time.ToInternalValue()));
  // The payload will still keep ownership of it's result dict, hence put a
  // a copy of the result dictionary here. This dictionary will be owned by
  // data_store_->cached_dict().
  dict->SetKey(kKeyResult, payload.result->Clone());

  return dict;
}

void WebserviceCache::TrimCache() {
  for (Cache::size_type i = cache_.size(); i > kWebserviceCacheMaxSize; i--) {
    Cache::reverse_iterator rbegin = cache_.rbegin();
    data_store_->cached_dict()->Remove(rbegin->first, NULL);
    cache_.Erase(rbegin);
  }
  data_store_->ScheduleWrite();
}

std::string WebserviceCache::PrependType(
    QueryType type, const std::string& query) {
  switch (type) {
    case WEBSTORE:
      return kWebstoreQueryPrefix + query;
    case PEOPLE:
      return kPeopleQueryPrefix + query;
    default:
      return query;
  }
}

WebserviceCache::Payload::Payload(const base::Time& time,
                                  std::unique_ptr<base::DictionaryValue> result)
    : time(time), result(std::move(result)) {}

WebserviceCache::Payload::Payload() = default;

WebserviceCache::Payload::~Payload() = default;

WebserviceCache::Payload& WebserviceCache::Payload::operator=(Payload&& other) {
  time = std::move(other.time);
  result = std::move(other.result);
  return *this;
}

}  // namespace app_list
