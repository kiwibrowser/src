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

#include "ondemand_supply_task.h"

#include <libaddressinput/address_field.h>
#include <libaddressinput/callback.h>
#include <libaddressinput/supplier.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "lookup_key.h"
#include "retriever.h"
#include "rule.h"
#include "util/size.h"

namespace i18n {
namespace addressinput {

OndemandSupplyTask::OndemandSupplyTask(
    const LookupKey& lookup_key,
    std::map<std::string, const Rule*>* rules,
    const Supplier::Callback& supplied)
    : hierarchy_(),
      pending_(),
      lookup_key_(lookup_key),
      rule_cache_(rules),
      supplied_(supplied),
      retrieved_(BuildCallback(this, &OndemandSupplyTask::Load)),
      success_(true) {
  assert(rule_cache_ != nullptr);
  assert(retrieved_ != nullptr);
}

OndemandSupplyTask::~OndemandSupplyTask() {
}

void OndemandSupplyTask::Queue(const std::string& key) {
  assert(pending_.find(key) == pending_.end());
  pending_.insert(key);
}

void OndemandSupplyTask::Retrieve(const Retriever& retriever) {
  if (pending_.empty()) {
    Loaded();
  } else {
    // When the final pending rule has been retrieved, the retrieved_ callback,
    // implemented by Load(), will finish by calling Loaded(), which will finish
    // by delete'ing this OndemandSupplyTask object. So after the final call to
    // retriever.Retrieve() no attributes of this object can be accessed (as the
    // object then no longer will exist, if the final callback has finished by
    // then), and the condition statement of the loop must therefore not use the
    // otherwise obvious it != pending_.end() but instead test a local variable
    // that isn't affected by the object being deleted.
    bool done = false;
    for (std::set<std::string>::const_iterator it = pending_.begin(); !done; ) {
      const std::string& key = *it++;
      done = it == pending_.end();
      retriever.Retrieve(key, *retrieved_);
    }
  }
}

void OndemandSupplyTask::Load(bool success,
                              const std::string& key,
                              const std::string& data) {
  size_t depth = std::count(key.begin(), key.end(), '/') - 1;
  assert(depth < size(LookupKey::kHierarchy));

  // Sanity check: This key should be present in the set of pending requests.
  size_t status = pending_.erase(key);
  assert(status == 1);  // There will always be one item erased from the set.
  (void)status;  // Prevent unused variable if assert() is optimized away.

  if (success) {
    // The address metadata server will return the empty JSON "{}" when it
    // successfully performed a lookup, but didn't find any data for that key.
    if (data != "{}") {
      Rule* rule = new Rule;
      if (LookupKey::kHierarchy[depth] == COUNTRY) {
        // All rules on the COUNTRY level inherit from the default rule.
        rule->CopyFrom(Rule::GetDefault());
      }
      if (rule->ParseSerializedRule(data)) {
        // Try inserting the Rule object into the rule_cache_ map, or else find
        // the already existing Rule object with the same ID already in the map.
        // It is possible that a key was queued even though the corresponding
        // Rule object is already in the cache, as the data server is free to do
        // advanced normalization and aliasing so that the ID of the data
        // returned is different from the key requested. (It would be possible
        // to cache such aliases, to increase performance in the case where a
        // certain alias is requested repeatedly, but such a cache would then
        // have to be kept to some limited size to not grow indefinitely with
        // every possible permutation of a name recognized by the data server.)
        std::pair<std::map<std::string, const Rule*>::iterator, bool> result =
            rule_cache_->insert(std::make_pair(rule->GetId(), rule));
        if (!result.second) {  // There was already an entry with this ID.
          delete rule;
        }
        // Pointer to object in the map.
        hierarchy_.rule[depth] = result.first->second;
      } else {
        delete rule;
        success_ = false;
      }
    }
  } else {
    success_ = false;
  }

  if (pending_.empty()) {
    Loaded();
  }
}

void OndemandSupplyTask::Loaded() {
  supplied_(success_, lookup_key_, hierarchy_);
  delete this;
}

}  // namespace addressinput
}  // namespace i18n
