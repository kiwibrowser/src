// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef I18N_ADDRESSINPUT_PRELOAD_SUPPLIER_H_
#define I18N_ADDRESSINPUT_PRELOAD_SUPPLIER_H_

#include <libaddressinput/callback.h>
#include <libaddressinput/supplier.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

class IndexMap;
class LookupKey;
class Retriever;
class Rule;
class Source;
class Storage;

// An implementation of the Supplier interface that owns a Retriever object,
// through which it can load aggregated address metadata for a region when
// instructed to, creating Rule objects and caching these. It also provides
// methods to check whether metadata for a particular region is already loaded
// or in progress of being loaded.
//
// When using a PreloadSupplier, it becomes possible to do synchronous address
// validation using an asynchronous Source, and to have full control over when
// network access is being done.
//
// The maximum size of this cache is naturally limited to the amount of data
// available from the data server. (Currently this is less than 12,000 items of
// in total less than 2 MB of JSON data.)
class PreloadSupplier : public Supplier {
 public:
  typedef i18n::addressinput::Callback<const std::string&, int> Callback;

  PreloadSupplier(const PreloadSupplier&) = delete;
  PreloadSupplier& operator=(const PreloadSupplier&) = delete;

  // Takes ownership of |source| and |storage|.
  PreloadSupplier(const Source* source, Storage* storage);
  ~PreloadSupplier() override;

  // Collects the metadata needed for |lookup_key| from the cache, then calls
  // |supplied|. If the metadata needed isn't found in the cache, it will call
  // the callback with status false.
  void Supply(const LookupKey& lookup_key,
              const Supplier::Callback& supplied) override;

  // Collects the metadata needed for |lookup_key| from the cache, by looking at
  // all available languages, then calls |supplied|. If the metadata needed
  // isn't found in the cache, it will call the callback with status false.
  void SupplyGlobally(const LookupKey& lookup_key,
                      const Supplier::Callback& supplied) override;

  // Should be called only when IsLoaded() returns true for the region code of
  // the |lookup_key|. Can return nullptr if the |lookup_key| does not
  // correspond to any rule data. The caller does not own the result.
  const Rule* GetRule(const LookupKey& lookup_key) const;

  // Loads all address metadata available for |region_code|. (A typical data
  // size is 10 kB. The largest is 250 kB.)
  //
  // If the rules are already in progress of being loaded, it does nothing.
  // Calls |loaded| when the loading has finished.
  void LoadRules(const std::string& region_code, const Callback& loaded);

  // Returns a mapping of lookup keys to rules. Should be called only when
  // IsLoaded() returns true for the |region_code|.
  const std::map<std::string, const Rule*>& GetRulesForRegion(
      const std::string& region_code) const;

  bool IsLoaded(const std::string& region_code) const;
  bool IsPending(const std::string& region_code) const;

 private:
  bool GetRuleHierarchy(const LookupKey& lookup_key, RuleHierarchy* hierarchy,
                        const bool search_globally) const;
  bool IsLoadedKey(const std::string& key) const;
  bool IsPendingKey(const std::string& key) const;

  const std::unique_ptr<const Retriever> retriever_;
  std::set<std::string> pending_;
  const std::unique_ptr<IndexMap> rule_index_;
  const std::unique_ptr<IndexMap> language_rule_index_;
  std::vector<const Rule*> rule_storage_;
  std::map<std::string, std::map<std::string, const Rule*> > region_rules_;
};

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_PRELOAD_SUPPLIER_H_
