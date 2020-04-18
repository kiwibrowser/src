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

#include <libaddressinput/ondemand_supplier.h>

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>

#include "lookup_key.h"
#include "ondemand_supply_task.h"
#include "region_data_constants.h"
#include "retriever.h"
#include "rule.h"

namespace i18n {
namespace addressinput {

OndemandSupplier::OndemandSupplier(const Source* source, Storage* storage)
    : retriever_(new Retriever(source, storage)) {
}

OndemandSupplier::~OndemandSupplier() {
  for (std::map<std::string, const Rule*>::const_iterator
       it = rule_cache_.begin(); it != rule_cache_.end(); ++it) {
    delete it->second;
  }
}

void OndemandSupplier::SupplyGlobally(const LookupKey& lookup_key,
                                      const Callback& supplied) {
  Supply(lookup_key, supplied);
}

void OndemandSupplier::Supply(const LookupKey& lookup_key,
                              const Callback& supplied) {
  OndemandSupplyTask* task =
      new OndemandSupplyTask(lookup_key, &rule_cache_, supplied);

  if (RegionDataConstants::IsSupported(lookup_key.GetRegionCode())) {
    size_t max_depth = std::min(
        lookup_key.GetDepth(),
        RegionDataConstants::GetMaxLookupKeyDepth(lookup_key.GetRegionCode()));

    for (size_t depth = 0; depth <= max_depth; ++depth) {
      const std::string& key = lookup_key.ToKeyString(depth);
      std::map<std::string, const Rule*>::const_iterator it =
          rule_cache_.find(key);
      if (it != rule_cache_.end()) {
        task->hierarchy_.rule[depth] = it->second;
      } else {
        task->Queue(key);  // If not in the cache, it needs to be loaded.
      }
    }
  }

  task->Retrieve(*retriever_);
}

}  // namespace addressinput
}  // namespace i18n
