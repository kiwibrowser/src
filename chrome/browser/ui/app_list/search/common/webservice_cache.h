// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_WEBSERVICE_CACHE_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_WEBSERVICE_CACHE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "chrome/browser/ui/app_list/search/dictionary_data_store.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class DictionaryValue;
}

namespace content {
class BrowserContext;
}

namespace app_list {

enum ResultStatus {
  FRESH = 0,
  STALE = 1
};

// Pair of values, first indicating whether this is a fresh result or stale,
// the second holding the actual value.
typedef std::pair<ResultStatus, const base::DictionaryValue*> CacheResult;

// WebserviceCache manages a cache of search results which should be valid
// during an input session. This will reduce unnecessary queries for typing
// backspace or so on. This is not meant to hold cache entries across multiple
// search sessions.
class WebserviceCache : public KeyedService,
                        public base::SupportsWeakPtr<WebserviceCache> {
 public:
  enum QueryType {
    WEBSTORE = 0,
    PEOPLE = 1
  };

  explicit WebserviceCache(content::BrowserContext* context);
  ~WebserviceCache() override;

  // Checks the current cache and returns the value for the |query| if it's
  // valid. Otherwise an CacheResult object with the result field set to NULL.
  // A query consists of a query 'type' and the query itself. The two current
  // types of queries supported are webstore queries and people search queries.
  // The type silos the query into it's own separate domain, preventing any
  // mixing of the queries.
  const CacheResult Get(QueryType type, const std::string& query);

  // Puts the new result to the query.
  void Put(QueryType type,
           const std::string& query,
           std::unique_ptr<base::DictionaryValue> result);

 private:
  struct Payload {
    Payload(const base::Time& time,
            std::unique_ptr<base::DictionaryValue> result);
    Payload();
    ~Payload();

    Payload& operator=(Payload&& other);

    base::Time time;
    std::unique_ptr<base::DictionaryValue> result;
  };

  using Cache = base::MRUCache<std::string, std::unique_ptr<Payload>>;

  // Callback for when the cache is loaded from the dictionary data store.
  void OnCacheLoaded(std::unique_ptr<base::DictionaryValue>);

  // Populates the payload parameter with the corresponding payload stored
  // in the given dictionary. If the dictionary is invalid for any reason,
  // this method will return false.
  bool PayloadFromDict(const base::DictionaryValue* dict, Payload* payload);

  // Returns a dictionary value for a given payload. The returned dictionary
  // will be owned by the data_store_ cached_dict, and freed on the destruction
  // of our dictionary datastore.
  std::unique_ptr<base::DictionaryValue> DictFromPayload(
      const Payload& payload);

  // Trims our MRU cache, making sure that any element that is removed is also
  // removed from the dictionary data store.
  void TrimCache();

  // Prepends a type string to the given query and returns a new query string.
  std::string PrependType(QueryType type, const std::string& query);

  Cache cache_;
  scoped_refptr<DictionaryDataStore> data_store_;

  bool cache_loaded_;

  DISALLOW_COPY_AND_ASSIGN(WebserviceCache);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_WEBSERVICE_CACHE_H_
